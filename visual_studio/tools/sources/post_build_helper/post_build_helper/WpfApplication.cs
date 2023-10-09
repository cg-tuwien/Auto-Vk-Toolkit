using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.IO;
using Hardcodet.Wpf.TaskbarNotification;
using CgbPostBuildHelper.Model;
using CgbPostBuildHelper.Utils;
using System.Text.RegularExpressions;
using CgbPostBuildHelper.View;
using CgbPostBuildHelper.ViewModel;
using System.Windows.Threading;
using System.Windows.Controls;
using System.Collections.ObjectModel;
using CgbPostBuildHelper.Deployers;
using System.Windows.Input;
using Newtonsoft.Json;
using System.Diagnostics;
using System.IO.Pipes;
using EnvDTE;

namespace CgbPostBuildHelper
{
	/// <summary>
	/// The only instance of our application which SingleApplicationInstance interfaces with
	/// in order to start new actions, instruct file watches, etc.
	/// </summary>
	class WpfApplication : Application, IMessageListLifetimeHandler
	{
		//static readonly Regex RegexFilterEntry = new Regex(@"<(None|Object|Image)\s+.*?Include\s*?\=\s*?\""(.*?)\""\s*?\>.*?\<Filter\s*?.*?\>(.*?)\<\/Filter\>.*?\<\/\1\>", 
		static readonly Regex RegexFilterEntry = new Regex(@"<(None|Object|Image)\s+[^\""]*?Include\s*?\=\s*?\""([^\""]*?)\""\s*?\>\s*?\<Filter\s*?\>([^\<]*?)\<\/Filter\>\s*?\<\/\1\>", 
			RegexOptions.Compiled | RegexOptions.Singleline | RegexOptions.IgnoreCase);

		/// <summary>
		/// Single instance of our TaskbarIcon
		/// </summary>
		private TaskbarIcon _taskbarIcon;
		private DateTime _messageListAliveUntil;
		private static readonly TimeSpan MessageListCloseDelay = TimeSpan.FromSeconds(2.0);
		private readonly MessagesListVM _messagesListVM;
		private readonly MessagesList _messagesListView;

		/// <summary>
		/// This one is important: 
		/// It stores all asset files associated with one build target. Upon HandleNewInvocation, the 
		/// file list is updated and those assets which HAVE CHANGED (based on a file-hash) are copied 
		/// to their respective target paths.
		/// 
		/// The Overseer will listen for process starts of those files, referenced in the keys. 
		/// In case, a process' full path name matches one of those keys, the file watcher for all its 
		/// child files will be launched.
		/// </summary>
		private readonly ObservableCollection<CgbAppInstanceVM> _instances = new ObservableCollection<CgbAppInstanceVM>();

		public ObservableCollection<CgbAppInstanceVM> AllInstances => _instances;

		private readonly Dictionary<CgbAppInstanceVM, Dictionary<string, Tuple<string, string>>> _toBeUpdated = new Dictionary<CgbAppInstanceVM, Dictionary<string, Tuple<string, string>>>();
		private readonly Dictionary<CgbAppInstanceVM, uint> _invocationWhichMayUpdate = new Dictionary<CgbAppInstanceVM, uint>();

		System.Drawing.Icon[] _icons = new System.Drawing.Icon[8];

		DispatcherTimer _iconAnimationTimer = null;
		int _animationRefCount = 0;
		int _iconAnimationIndex = 0;

		readonly Dispatcher _myDispatcher;

		public WpfApplication()
		{
			_myDispatcher = Dispatcher.CurrentDispatcher;
			_messagesListVM = new MessagesListVM(this);

			_messagesListView = new MessagesList()
			{
				Width = 420,
				LifetimeHandler = this,
				DataContext = _messagesListVM
			};

			_icons[0] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon.ico")).Stream);
			_icons[1] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot1.ico")).Stream);
			_icons[2] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot2.ico")).Stream);
			_icons[3] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot3.ico")).Stream);
			_icons[4] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot4.ico")).Stream);
			_icons[5] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot5.ico")).Stream);
			_icons[6] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot6.ico")).Stream);
			_icons[7] = new System.Drawing.Icon(System.Windows.Application.GetResourceStream(new Uri("pack://application:,,,/tray_icon_dot7.ico")).Stream);

			var rd = new ResourceDictionary()
			{
				Source = new Uri(";component/ContextMenuResources.xaml", UriKind.RelativeOrAbsolute)
			};

			_taskbarIcon = new TaskbarIcon()
			{
				Icon = _icons[0],
				ToolTipText = "Post Build Helper",
				ContextMenu = (ContextMenu)rd["SysTrayMenu"]
			};

			var contextMenuActions = new ContextMenuActionsVM(this);
			_taskbarIcon.ContextMenu.DataContext = contextMenuActions;
			_taskbarIcon.LeftClickCommand = new DelegateCommand(_ => 
			{
				ShowMessagesList();
			});
			_taskbarIcon.DoubleClickCommand = new DelegateCommand(_ =>
			{
				ShowMessagesList();
				contextMenuActions.ShowInstances.Execute(null);
			});
		}

		private void DispatcherInvokeLater(TimeSpan delay, Action action)
		{
			var timer = new DispatcherTimer { Interval = delay };
			timer.Start();
			timer.Tick += (sender, args) =>
			{
				timer.Stop();
				action();
			};
		}

		public void EndAllWatches()
		{
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				foreach (var inst in AllInstances)
				{
					ClearAllFileWatchers(inst);
				}
			}));
		}

		public void EndAllWatchesAndExitApplication()
		{
			EndAllWatches();

			ShutdownNamedPipeServer();

			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				var timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1.0) };
				timer.Start();
				timer.Tick += (sender, args) =>
				{
					timer.Stop();
					Application.Current.Shutdown();
				};
			}));
		}

        #region Status Named Pipe

        private readonly object _namedPipeSync = new object();
		private bool _shutDownPipeServer = false;
		private string _applicationStatus = string.Empty;

		public void ShutdownNamedPipeServer()
        {
			lock(_namedPipeSync)
            {
				_applicationStatus = "Shutting down...";
				_shutDownPipeServer = true;
            }
        }

		public void NamedPipeServer()
        {
			while(!_shutDownPipeServer)
			{ 
			    using (NamedPipeServerStream pipeServer = new NamedPipeServerStream("Gears-Vk Application Status Pipe", PipeDirection.Out))
			    {
    				// Wait for a client to connect
    				Console.Write("Waiting for client connection...");
                
                    pipeServer.WaitForConnection();

				    Console.WriteLine("Client connected.");
				    try
				    {
						
					    using (StreamWriter sw = new StreamWriter(pipeServer))
					    {
						    sw.AutoFlush = true;
							lock(_namedPipeSync)
                            {
						        sw.WriteLine(_applicationStatus);
                            }
					    }
				    }
				    // Catch the IOException that is raised if the pipe is broken
				    // or disconnected.
				    catch (IOException e)
				    {
					    Console.WriteLine("ERROR: {0}", e.Message);
				    }
                }
			}
		}

        #endregion

		public void StartAnimateIcon()
		{
            // Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				if (null != _iconAnimationTimer)
				{
					_animationRefCount++;
					return;
				}

				_iconAnimationTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(70.0) };
				_iconAnimationTimer.Tick += (sender, args) =>
				{
					_iconAnimationIndex = (_iconAnimationIndex + 1) % _icons.Length;
					_taskbarIcon.Icon = _icons[_iconAnimationIndex];
				};
				_animationRefCount = 1;
				_iconAnimationTimer.Start();
			}));
		}

		public void StopAnimateIcon()
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_animationRefCount = Math.Max(_animationRefCount - 1, 0);
				if (null == _iconAnimationTimer || _animationRefCount > 0)
				{
					return;
				}

				_iconAnimationTimer.Stop();
				_iconAnimationTimer = null;
				_iconAnimationIndex = 0;
				_taskbarIcon.Icon = _icons[_iconAnimationIndex];

				// Reset application status after a while:
				var resetTimer = new DispatcherTimer(DispatcherPriority.Normal);
				resetTimer.Tick += (sender, args) =>
                {
					resetTimer.Stop(); // Execute only once.. but after a delay!
					lock (_namedPipeSync)
					{
				        if (null == _iconAnimationTimer || _animationRefCount > 0)
                        {
                            _applicationStatus = string.Empty;
                        }
					}
				};
				resetTimer.Interval = TimeSpan.FromSeconds(0.5);
				resetTimer.Start();
            }));
		}

		public void CloseMessagesListLater(bool setAliveTime)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				if (setAliveTime)
				{
					_messageListAliveUntil = DateTime.Now + MessageListCloseDelay;
				}

				DispatcherInvokeLater(MessageListCloseDelay, () =>
				{
					if (_messageListAliveUntil <= DateTime.Now)
					{
						_taskbarIcon.CloseBalloon();
					}
					else if (_messageListAliveUntil != DateTime.MaxValue)
					{
						CloseMessagesListLater(false);
					}
				});
			}));
		}

		public void CloseMessagesListNow()
		{
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_messageListAliveUntil = DateTime.Now - TimeSpan.FromSeconds(1.0);
				_taskbarIcon.CloseBalloon();
			}));
		}

		protected override void OnStartup(StartupEventArgs e)
		{
			base.OnStartup(e);
		}

		/// <summary>
		/// The single application instance is getting teared down => cleanup!
		/// </summary>
		protected override void OnExit(ExitEventArgs e)
		{
			_taskbarIcon.Dispose();
			base.OnExit(e);
		}

		/// <summary>
		/// Handles ONE file deployment
		/// </summary>
		/// <param name="config">The build's config parameters</param>
		/// <param name="filePath">Path to the input file</param>
		/// <param name="filterPath">Associated filter path of the file</param>
		/// <param name="modDeployments">List of deployments; this will be modified, i.e. a deployment will be added to this list in the success-case</param>
		/// <param name="modFileDeployments">List of FILE deployments; this will be modified. A deployment can affect multiple files, which are all added to this list in the success-case.</param>
		/// <param name="modWindowsToShowFor">List of single file deployments which an emergency-window has to be opened for; this will be modified.</param>
		public void HandleFileToDeploy(
			InvocationParams config, string filePath, string filterPath,
			IList<IFileDeployment> modDeployments, IList<FileDeploymentData> modFileDeployments, IList<FileDeploymentData> modWindowsToShowFor,
			bool doNotOverwriteExisting = false)
		{
			IFileDeployment savedForUseInException = null;
			try
			{
				lock (_namedPipeSync)
				{
					_applicationStatus = $"Deploying files from '{new FileInfo(config.VcxprojPath).Name}' (currently working on '{new FileInfo(filePath).Name}')...";
				}

				savedForUseInException = null;
				CgbUtils.PrepareDeployment(
					config,
					filePath, filterPath,
					out var deployment);

				// It can be null, if it is not an asset/shader that should be deployed
				if (null == deployment)
				{
					return;
				}

				savedForUseInException = deployment;

				// Do it!
				if (doNotOverwriteExisting)
				{
					// ...unless we shouldn't do it!
					if (File.Exists(deployment.DesignatedOutputPath))
					{
                        Trace.TraceInformation($"Not deploying file to '{deployment.DesignatedOutputPath}' because it already exists (and don't overwrite flag is set).");
						return;
					}
				}

				// Before deploying, check for conflicts and try to resolve them (which actually is only implemented for Models)
				bool repeatChecks;
				var deploymentBase = (DeploymentBase)deployment;
				var deploymentModel = deployment as ModelDeployment;
				var deploymentsToIssueWarningsFor = new HashSet<DeploymentBase>();
				do
				{
					repeatChecks = false;
					foreach (var otherDeployment in modDeployments)
					{
						var otherDeploymentBase = (DeploymentBase)otherDeployment;
						if (deploymentBase.HasConflictWith(otherDeploymentBase))
						{
							if (null != deploymentModel && otherDeployment is ModelDeployment otherDeploymentModel)
							{
								deploymentModel.ResolveConflictByMovingThisToSubfolder();
								repeatChecks = true;
								break;
							}
							else
							{
								deploymentsToIssueWarningsFor.Add(otherDeploymentBase);
							}
						}
					}
				} while (repeatChecks);

				// Do we have to show any warnings?
				if (deploymentsToIssueWarningsFor.Count > 0)
				{
					foreach (var otherDeploymentBase in deploymentsToIssueWarningsFor)
					{
						AddToMessagesList(Message.Create(MessageType.Warning, $"File '{deploymentBase.InputFilePath}' has an unresolvable conflict with file '{otherDeploymentBase.InputFilePath}'.", null, deploymentBase.InputFilePath) /* TODO: ADD INSTANCE HERE */);
					}
					ShowMessagesList();
				}

				// And, before deploying, try to optimize!
				foreach (var otherDeployment in modDeployments)
				{
					System.Diagnostics.Debug.Assert(deployment != otherDeployment);
					var otherDeploymentBase = (DeploymentBase)otherDeployment;
					if (null != deploymentModel && otherDeployment is ModelDeployment otherDeploymentModel)
					{
						deploymentModel.KickOutAllTexturesWhichAreAvailableInOther(otherDeploymentModel);
					}
				}

				deployment.Deploy();

				foreach (var deployedFile in deployment.FilesDeployed)
				{
					// For the current files list:
					modFileDeployments.Add(deployedFile);

					var deploymentHasErrors = deployedFile.Messages.ContainsMessagesOfType(MessageType.Error);
					var deploymentHasWarnings = deployedFile.Messages.ContainsMessagesOfType(MessageType.Warning);

					// Show errors/warnings in window immediately IF this behavior has been opted-in via our settings
					if (deploymentHasWarnings || deploymentHasErrors)
					{
						if ((CgbPostBuildHelper.Properties.Settings.Default.ShowWindowForVkShaderDeployment && deployment is Deployers.VkShaderDeployment)
							|| (CgbPostBuildHelper.Properties.Settings.Default.ShowWindowForGlShaderDeployment && deployment is Deployers.GlShaderDeployment)
							|| (CgbPostBuildHelper.Properties.Settings.Default.ShowWindowForModelDeployment && deployment is Deployers.ModelDeployment))
						{
							modWindowsToShowFor.Add(deployedFile);
						}
					}
				}

				modDeployments.Add(deployment);
			}
			catch (UnauthorizedAccessException uaex)
			{
				if (uaex.Message.IndexOf(".dll", StringComparison.InvariantCultureIgnoreCase) >= 0 
					&& null != savedForUseInException
					&& 0 != savedForUseInException.FilesDeployed.Count
					&& CgbPostBuildHelper.Properties.Settings.Default.HideAccessDeniedErrorsForDlls)
				{
					savedForUseInException.FilesDeployed.Last().Messages.Add(Message.Create(MessageType.Information, uaex.Message, null));
					foreach (var deployedFile in savedForUseInException.FilesDeployed)
					{
						modFileDeployments.Add(deployedFile);
					}
					modDeployments.Add(savedForUseInException);
				}
				else
				{
					AddToMessagesList(Message.Create(MessageType.Error, uaex.Message, null), config); // TODO: Window with more info?
					ShowMessagesList();
				}
			}
			catch (Exception ex)
			{
				AddToMessagesList(Message.Create(MessageType.Error, ex.Message, null), config); // TODO: Window with more info?
				ShowMessagesList();
			}
		}

		public void UpdateViewAfterHandledInvocationAndStartFileSystemWatchers(CgbEventType eventType, InvocationParams config, IList<IFileDeployment> deployments, IList<FileDeploymentData> fileDeployments, IList<FileDeploymentData> windowsToShowFor, bool stopAnimateIcon = true)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				// See, if we're already handling that executable!
				var inst = _instances.GetInstance(config.ExecutablePath);

				// Create new or update config/invocation params:
				if (null == inst)
				{
					inst = new CgbAppInstanceVM
					{
						Config = config
					};
					_instances.Add(inst);

					AddToMessagesList(Message.Create(MessageType.Information, $"Handling new instance '{inst.ShortPath}' at path '{inst.Path}'", () =>
					{
						var wnd = new View.InstancesList
						{
							DataContext = new { Items = _instances }
						};
						wnd.ContentRendered += (object sender, EventArgs e) =>
						{
							var children = wnd.GetChildren(true);
							foreach (var child in children)
							{
								if (!(child is FrameworkElement feChild)) continue;
								if (feChild.DataContext == inst)
								{
									feChild.BringIntoView();
									break;
								}
							}
						};
						wnd.Show();
					}), inst);
				}
				else
				{
					if (CgbEventType.Build == eventType) // only update the data for "Build" events
					{
						// Before we move on, tear down all the previous file system watchers for this instance
						ClearAllFileWatchers(inst);

						inst.Config = config;
						// Proceed with an empty list because files could have changed:
						inst.Files.Clear();
						inst.DependencyMapping.Clear();
					}
				}

				// Create an event, add all those files, and handle all those messages
				var evnt = new CgbEventVM(eventType);

                foreach (var depl in deployments)
                {
					foreach (var fd in depl.FilesDeployed)
                    {
					    if (fd.DeploymentType == DeploymentType.Dependency)
                        {
                            if (CgbEventType.Build == eventType)
                            {
                                var dependees = (from x in depl.FilesDeployed where x != fd select new FileDeploymentDataVM(x, inst)).ToList();
                                var np = CgbUtils.NormalizePath(fd.InputFilePath);
                                if (inst.DependencyMapping.ContainsKey(np))
                                {
                                    inst.DependencyMapping[np].AddRange(dependees);
                                }
                                else
                                {
                                    inst.DependencyMapping.Add(np, dependees);
									// We need it in the list of files once so that FileWatcher is being set up
                                    inst.Files.Insert(0, new FileDeploymentDataVM(fd, inst));
                                }
                            }
                        }
                    }
				}

				var eventHasErrors = false;
				var eventHasWarnings = false;
				var eventHasInfos = false;
				foreach (var fd in fileDeployments)
				{
                    eventHasErrors = eventHasErrors || fd.Messages.ContainsMessagesOfType(MessageType.Error);
					eventHasWarnings = eventHasWarnings || fd.Messages.ContainsMessagesOfType(MessageType.Warning);
					eventHasInfos = eventHasInfos || fd.Messages.ContainsMessagesOfType(MessageType.Information);

                    if (fd.DeploymentType == DeploymentType.Dependency)
                    {
						continue; // Already handled right above
                    }

                    evnt.Files.Insert(0, new FileDeploymentDataVM(fd, inst));

					if (CgbEventType.Build == eventType)
					{
						inst.Files.Add(new FileDeploymentDataVM(fd, inst));
					}
				}
				// Insert at the top
				inst.AllEventsEver.Insert(0, evnt);

				// Count all which are NOT a Dependency:
				var numFileDeploymentsWithoutDependencies = (from x in fileDeployments where x.DeploymentType != DeploymentType.Dependency select x).Count();

				// Create those tray-messages
				if (eventHasErrors || eventHasWarnings)
				{
					void addErrorWarningsList(MessageType msgType, string message, string title)
					{
						AddToMessagesList(Message.Create(msgType, message, () =>
						{
							var window = new View.WindowToTheTop
							{
								Width = 960, Height = 600,
								Title = title
							};
							window.InnerContent.Content = new EventFilesView()
							{
								DataContext = inst
							};
							window.ContentRendered += (object sender, EventArgs e) =>
							{
								var children = window.GetChildren(true);
								foreach (var child in children)
								{
									if (!(child is FrameworkElement feChild)) continue;
									if (feChild.DataContext == evnt)
									{
										feChild.BringIntoView();
										break;
									}
								}
							};
							window.Show();
						}), inst);
					}

					if (eventHasWarnings && eventHasErrors)
					{
						addErrorWarningsList(MessageType.Error, $"Deployed {numFileDeploymentsWithoutDependencies} files with ERRORS and WARNINGS.", "Build-Event Details including ERRORS and WARNINGS");
					}
					else if (eventHasWarnings)
					{
						addErrorWarningsList(MessageType.Warning, $"Deployed {numFileDeploymentsWithoutDependencies} files with WARNINGS.", "Build-Event Details including WARNINGS");
					}
					else
					{
						addErrorWarningsList(MessageType.Error, $"Deployed {numFileDeploymentsWithoutDependencies} files with ERRORS.", "Build-Event Details including ERRORS");
					}
					ShowMessagesList();
				}
				else
				{
					var msgType = MessageType.Success;
					var additionalInfo = "";
					if (eventHasInfos)
					{
						msgType = MessageType.Information;
						additionalInfo = " with INFOS";
					}

					AddToMessagesList(Message.Create(msgType, $"Deployed {numFileDeploymentsWithoutDependencies} files{additionalInfo}.", () =>
					{
						var window = new View.WindowToTheTop
						{
							Width = 960, Height = 600,
							Title = "Build-Event Details"
						};
						window.InnerContent.Content = new EventFilesView()
						{
							DataContext = inst
						};
						window.ContentRendered += (object sender, EventArgs e) =>
						{
							var children = window.GetChildren(true);
							foreach (var child in children)
							{
								if (!(child is FrameworkElement feChild)) continue;
								if (feChild.DataContext == evnt)
								{
									feChild.BringIntoView();
									break;
								}
							}
						};
						window.Show();
					}), inst);
				}

				// Show the "emergency windows"
				foreach (var fd in windowsToShowFor)
				{
					var vm = new FileDeploymentDataVM(fd, inst);
					var window = new View.WindowToTheTop
					{
						Width = 480, Height = 320,
						Title = "Messages for file " + fd.InputFilePath
					};
					window.InnerContent.Content = new MessagesList()
					{
						DataContext = new 
                        { 
                            Items = vm.Messages,
                            DismissCommand = new Func<ICommand>(() => null)() // Evaluate to null in order to set the >>>DISMISS>>> buttons to collapsed}
                        }
                    };
					window.Show();
				}

				if (CgbEventType.Build == eventType) // only mess around with file system watchers at "Build" events
				{
					if (!CgbPostBuildHelper.Properties.Settings.Default.DoNotMonitorFiles)
					{
						// Finally, start watching those files
						StartWatchingDeployedFiles(inst);
					}
				}

				if (stopAnimateIcon)
				{
					StopAnimateIcon();
				}
			}));
		}

		/// <summary>
		/// Handle a new invocation (usually triggerd by a post build step out of VisualStudio)
		/// </summary>
		/// <param name="p">All the parameters passed by that invocation/post build step</param>
		public void HandleNewInvocation(InvocationParams config)
		{
			lock (_namedPipeSync)
			{
				_applicationStatus = $"Analyzing files to deploy from '{new FileInfo(config.VcxprojPath).Name}'...";
			}

			StartAnimateIcon();

            // A story of a pathetic developer who learned that exceptions on background
            // threads are just swallowed and are going into Nirvana:
            //
            // cgb_post_build_helper.exe from the visual_studio/executables/ directory would
            // always silently fail somewhere, BUT the application wouldn't quit! As it seems,
            // ONLY the background thread (started with Task.Run) throws up and is terminated.
            //
            // So the pathetic developer suspected some problem with the asynchronous background
            // thread, started via Task.Run. It turned out that just Newtonsoft.Json.dll was missing.
            // However, you don't find out that it is missing, because the application won't tell you! WTF?!??!
            // Only the background thread requires it, and it seems like only the background thread 
            // is terminated IF it is missing.
            // 
            // This is the reason why everything worked when started from within visual studio.
            // Started from the visual_studio/executables/ directory, it would just SILENTLY fail.
            // Removing the following Task.Run and having the code execute synchronously finally 
            // revealed the error. *facepalm*
            var task = Task.Run(() =>
            {
                Trace.TraceInformation($"Handling new invocation for {config.VcxprojPath}");
                Trace.Flush();

                // Parse the .filters file for asset files and shader files
                var filtersFile = new FileInfo(config.FiltersPath);
				var filtersContent = File.ReadAllText(filtersFile.FullName);
				var filters = RegexFilterEntry.Matches(filtersContent);
				int n = filters.Count;

				// -> Store the whole event (but a little bit later)
				var cgbEvent = new CgbEventVM(CgbEventType.Build);

				var deployments = new List<IFileDeployment>();
				var fileDeployments = new List<FileDeploymentData>();
				var windowsToShowFor = new List<FileDeploymentData>();

				// Perform sanity check before actually evaluating the individual files:
				try 
				{
                    //var test = CgbUtils.NormalizePartialPath("  /\\/\\asdfa/sdf/sdf/sdf\\asdf \\ asdfsdf?/\\ ");
                    Trace.TraceInformation($"stage 0 {config.VcxprojPath}");

                    var setOfFilters = new HashSet<string>();
					var filesToCheck = new List<string>();
					for (int i=0; i < n; ++i)
					{
						Match match = filters[i];
						var fileName = match.Groups[2].Value;
						var filterPath = match.Groups[3].Value;
						setOfFilters.Add(CgbUtils.NormalizePartialPath(filterPath));

						var filterPlusFile = Path.Combine(filterPath, new FileInfo(fileName).Name);
						filesToCheck.Add(filterPlusFile);
					}

					foreach (var entry in filesToCheck)
					{
						if (setOfFilters.Contains(CgbUtils.NormalizePartialPath(entry)))
						{
							AddToMessagesList(Message.Create(MessageType.Error, 
								$"A filter must not have the same name as one of its files => ABORTING! " +
								$"This restriction is a neccessity for conflict handling. Please fix your filters!", 
								() => {
									CgbUtils.ShowDirectoryInExplorer(config.FiltersPath);
								}), config);

							StopAnimateIcon(); // <-- Never forgetti
							return;
						}
					}
				}
				catch (Exception ex)
				{
					AddToMessagesList(Message.Create(MessageType.Error,
						$"An unexpected error occured during sanity checking the .filters file. Message: '{ex.Message}'",
						() => {
							CgbUtils.ShowDirectoryInExplorer(config.FiltersPath);
						}), config);
				}
                Trace.TraceInformation($"stage 1 {config.VcxprojPath}");

                // I can't believe what I've programmed here :-/
                // For now, I'm just going to hack the .fscene parser into here. In future, this should be refactored so that the whole code structure makes more sense.
                var FilesAndFilters = new List<Tuple<string, string>>();

				// -> Parse the .filters file and deploy each and every file
				for (int i=0; i < n; ++i)
				{
					Match match = filters[i];

					string filePath;
					string filterPath;
					try
					{
						filePath = Path.Combine(filtersFile.DirectoryName, match.Groups[2].Value);
						filterPath = match.Groups[3].Value;
                        FilesAndFilters.Add(new Tuple<string, string>(filePath, filterPath));
                    }
					catch (Exception ex)
					{
                        Trace.TraceError("Skipping file, because: " + ex.Message);
						continue;
					}
				}
                Trace.TraceInformation($"stage 2 {config.VcxprojPath}");

                // Handle all ORCA files
                // Is it an .fscene file?
                for (int i = 0; i < FilesAndFilters.Count; ++i)
                {
                    var fsceneFile = new FileInfo(FilesAndFilters[i].Item1);
                    var fsceneFilter = FilesAndFilters[i].Item2;
                    if (!fsceneFile.Exists)
                    {
                        Trace.TraceError($"File '{FilesAndFilters[i].Item1}' does not exist");
                        continue;
                    }

                    if (fsceneFile.FullName.Trim().EndsWith(".fscene", true, System.Globalization.CultureInfo.InvariantCulture))
                    {
                        try
                        {
                            // Extract data out of it:
                            var referencedModels = new HashSet<string>();
                            var referencedLightProbes = new HashSet<string>();
                            var referencedSkyboxes = new HashSet<string>();

                            dynamic fscene = JsonConvert.DeserializeObject(File.ReadAllText(fsceneFile.FullName));

                            var models = fscene.models;
                            if (null != models)
                            {
                                foreach (var model in models)
                                {
                                    string path = model.file;
                                    referencedModels.Add(path);
                                }
                            }

                            var lightProbes = fscene.light_probes;
                            if (null != lightProbes)
                            {
                                foreach (var lightProbe in lightProbes)
                                {
                                    string path = lightProbe.file;
                                    referencedLightProbes.Add(path);
                                }
                            }

                            var userDefined = fscene.user_defined;
                            if (null != userDefined && null != userDefined.sky_box)
                            {
                                string path = userDefined.sky_box;
                                referencedSkyboxes.Add(path);
                            }

                            // Add all the extracted data to the FilesAndFilters collection:
                            foreach (var referencedModel in referencedModels)
                            {
								var pathInFscene = Path.GetDirectoryName(referencedModel);
								var modelFilter = Path.Combine(fsceneFilter, pathInFscene);
                                FilesAndFilters.Insert(i++, new Tuple<string, string>(Path.Combine(fsceneFile.DirectoryName, referencedModel), modelFilter));
                            }
                            foreach (var referencedLightProbe in referencedLightProbes)
                            {
                                var pathInFscene = Path.GetDirectoryName(referencedLightProbe);
                                var lightProbeFilter = Path.Combine(fsceneFilter, pathInFscene);
                                FilesAndFilters.Insert(i++, new Tuple<string, string>(Path.Combine(fsceneFile.DirectoryName, referencedLightProbe), lightProbeFilter));
                            }
                            foreach (var referencedSkybox in referencedSkyboxes)
                            {
								var pathInFscene = Path.GetDirectoryName(referencedSkybox);
                                var skyboxFilter = Path.Combine(fsceneFilter, pathInFscene);
                                FilesAndFilters.Insert(i++, new Tuple<string, string>(Path.Combine(fsceneFile.DirectoryName, referencedSkybox), skyboxFilter));
                            }
                        }
                        catch (JsonException jex)
                        {
                            Trace.TraceError(jex.Message);
                        }
                    }
                }
                Trace.TraceInformation($"stage 3 {config.VcxprojPath}");

                foreach (var tpl in FilesAndFilters)
                {
                    HandleFileToDeploy(config, tpl.Item1, tpl.Item2, deployments, fileDeployments, windowsToShowFor);
					
                }
                Trace.TraceInformation($"stage 4 {config.VcxprojPath}");

                // In addition, deploy the DLLs from the framework's external directory!
                foreach (var configCgbExternalPath in config.CgbExternalPaths)
                {
                    var pathToExtBinDlls = Path.Combine(
                        configCgbExternalPath, 
						CgbPostBuildHelper.Properties.Settings.Default.AlwaysDeployReleaseDlls 
							? CgbPostBuildHelper.Properties.Settings.Default.ReleaseSubPathInExternals
							: config.Configuration == BuildConfiguration.Debug
								? CgbPostBuildHelper.Properties.Settings.Default.DebugSubPathInExternals
								: CgbPostBuildHelper.Properties.Settings.Default.ReleaseSubPathInExternals,
						"bin",
						config.Platform.BuildPlatformToPartOfPath());

					var dirInfo = new DirectoryInfo(pathToExtBinDlls);
					if (!dirInfo.Exists)
					{
						AddToMessagesList(Message.Create(MessageType.Error, $"Path to framework's externals does not exist?! This one => '{dirInfo.FullName}'", () =>
						{
							CgbUtils.ShowDirectoryInExplorer(configCgbExternalPath);
						}), config);
					}
					
					var allFiles = dirInfo.EnumerateFiles("*.*", SearchOption.AllDirectories);
					foreach (var file in allFiles)
					{
						HandleFileToDeploy(config, file.FullName, ".", deployments, fileDeployments, windowsToShowFor);
					}
				}

                // TODO: Test DLL deployment!!
                Trace.TraceInformation($"stage 5 {config.VcxprojPath}");

                // Do the things which must be done on the UI thread:
                UpdateViewAfterHandledInvocationAndStartFileSystemWatchers(CgbEventType.Build, config, deployments, fileDeployments, windowsToShowFor);

				lock (_namedPipeSync)
				{
					if (_applicationStatus.Contains(new FileInfo(config.VcxprojPath).Name))
                    {
						_applicationStatus = string.Empty;
                    }
				}
			});

		}

		public void HandleFileEvent(string filePath, CgbAppInstanceVM inst, ObservableCollection<WatchedFileVM> files)
		{
			// Ignore temp-files
			if (filePath.Contains("~"))
			{
				return;
			}

			// Are we even watching for this file?
			var watchedFileEntry = files.GetFile(filePath);
			if (null == watchedFileEntry)
			{
				return;
			}

			StartAnimateIcon();
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				if (!_toBeUpdated.ContainsKey(inst))
				{
					_toBeUpdated.Add(inst, new Dictionary<string, Tuple<string, string>>());
				}

				var nrmFilename = CgbUtils.NormalizePath(filePath);
				var isDependency = inst.DependencyMapping.ContainsKey(nrmFilename);
				var added = false;
				if (isDependency)
                {
                    foreach (var fileDeploymentDataVm in inst.DependencyMapping[nrmFilename])
                    {
						var nrmDepp = CgbUtils.NormalizePath(fileDeploymentDataVm.InputFilePath);
						if (!_toBeUpdated[inst].ContainsKey(nrmDepp))
                        {
                            // to be updated!!
                            _toBeUpdated[inst].Add(nrmDepp, Tuple.Create(fileDeploymentDataVm.InputFilePath, fileDeploymentDataVm.FilterPath));
                            added = true;
                        }
					}
                }
				else if (!_toBeUpdated[inst].ContainsKey(nrmFilename))
				{
					// to be updated!!
					_toBeUpdated[inst].Add(nrmFilename, Tuple.Create( filePath, watchedFileEntry.FilterPath ));
					added = true;
				}
				
				if (!added)
				{
					// nothing to do
					StopAnimateIcon();
					return;
				}

				// only one can be the updater!
                if (!_invocationWhichMayUpdate.ContainsKey(inst))
                {
                    _invocationWhichMayUpdate.Add(inst, 0);
                }
                _invocationWhichMayUpdate[inst] += 1;
				var myId = _invocationWhichMayUpdate[inst];


                var timer = new DispatcherTimer 
				{ 
					Interval = TimeSpan.FromMilliseconds(200.0) // This is somewhat of a random/magic number, but it should be fine, I guess
				};
				timer.Start();
				timer.Tick += (sender, args) =>
				{
					timer.Stop();
					
					if (myId == _invocationWhichMayUpdate[inst])
					{
						// I am the one!
						while (_toBeUpdated.Count > 0)
						{
							var nextRandomKey = _toBeUpdated.Keys.First();
						    var updConfig = nextRandomKey.Config;
							var item = _toBeUpdated[nextRandomKey];
							_toBeUpdated.Remove(nextRandomKey);
							Task.Run(() =>
							{
								var deployments = new List<IFileDeployment>();
								var fileDeployments = new List<FileDeploymentData>();
								var windowsToShowFor = new List<FileDeploymentData>();

								// -> Deploy each file for this update event
								foreach (var fileInfos in item)
								{
									var updFilePath = fileInfos.Value.Item1;
									var updFilterPath = fileInfos.Value.Item2;

									if (null == updFilePath || null == updFilterPath)
                                    {
										continue;
                                    }

									HandleFileToDeploy(updConfig, updFilePath, updFilterPath, deployments, fileDeployments, windowsToShowFor);
								}

								// Do the things which must be done on the UI thread:
								UpdateViewAfterHandledInvocationAndStartFileSystemWatchers(CgbEventType.Update, updConfig, deployments, fileDeployments, windowsToShowFor);

								StopAnimateIcon(); // I think, we're calling it too often, now
							});
						}
					}
					else
					{
						// Someone else will update!
						StopAnimateIcon();
					}
				};
			}));
		}

		public void ClearAllFileWatchers(CgbAppInstanceVM inst)
		{
			foreach (var fw in inst.CurrentlyWatchedDirectories)
			{
				fw.EndWatchAndDie();
			}
			inst.CurrentlyWatchedDirectories.Clear();
		}

		public void StartWatchingDeployedFiles(CgbAppInstanceVM inst)
		{
			var watches = new Dictionary<string, List<WatchedFileVM>>();
			foreach (var fw in inst.Files)
			{
				// Skip symlinks:
				if (fw.DeploymentType == DeploymentType.Symlink)
				{
					continue;
				}

				var fi = new FileInfo(fw.InputFilePath);
				var existingKey = (from k in watches.Keys 
									where CgbUtils.NormalizePath(k) == CgbUtils.NormalizePath(fi.DirectoryName)
									select k)
									.FirstOrDefault();

				if (null == existingKey)
				{
					existingKey = fi.DirectoryName;
					watches.Add(existingKey, new List<WatchedFileVM>());
				}
				watches[existingKey].Add(new WatchedFileVM(fw.FilterPath, fw.InputFilePath));
			}

			foreach (var w in watches)
			{
				try 
				{
				    var watchDir = new WatchedDirectoryVM(this, inst, w.Key);

				    foreach (var f in w.Value)
				    {
					    watchDir.Files.Add(f);
				    }

				    // Start to watch:
				    inst.CurrentlyWatchedDirectories.Add(watchDir);
				    watchDir.NightGathersAndNowMyWatchBegins();
				}
				catch (Exception ex) 
                {
                    AddToMessagesList(Message.Create(MessageType.Error, $"Failed to establish FileSystemWatcher for {w.Key}", () =>
                    {
                        MessageBox.Show(ex.Message);
                    }), inst);
				}
			}
		}

		public void AddToMessagesList(Model.Message msg, InvocationParams config)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_messagesListVM.Items.Add(new MessageVM(_instances, config, msg));
				RemoveOldMessagesFromList();
				ShowMessagesList();
			}));
		}

		public void AddToMessagesList(IEnumerable<Model.Message> msgs, InvocationParams config)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				foreach (var msg in msgs)
				{
					_messagesListVM.Items.Add(new MessageVM(_instances, config, msg));
				}
				RemoveOldMessagesFromList();
				ShowMessagesList();
			}));
		}

		public void AddToMessagesList(Model.Message msg, CgbAppInstanceVM inst = null)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_messagesListVM.Items.Add(new MessageVM(inst, msg));
				RemoveOldMessagesFromList();
				ShowMessagesList();
			}));
		}

		public void AddToMessagesList(IEnumerable<Model.Message> msgs, CgbAppInstanceVM inst = null)
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				foreach (var msg in msgs)
				{
					_messagesListVM.Items.Add(new MessageVM(inst, msg));
				}
				RemoveOldMessagesFromList();
				ShowMessagesList();
			}));
		}

		public void RemoveOldMessagesFromList()
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				var cutoffDate = DateTime.Now - TimeSpan.FromMinutes(10.0);
				for (int i = _messagesListVM.Items.Count - 1; i >= 0; --i)
				{
					if (_messagesListVM.Items[i].CreateDate < cutoffDate)
					{
						_messagesListVM.Items.RemoveAt(i);
					}
				}
			}));
		}

		public void ShowMessagesList()
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
                Trace.TraceInformation("Show Messages list NOW " + DateTime.Now);

				foreach (var msgVm in _messagesListVM.Items)
				{
					msgVm.IssueAllOnPropertyChanged();
				}

				_taskbarIcon.ShowCustomBalloon(_messagesListView, System.Windows.Controls.Primitives.PopupAnimation.None, null);
				CloseMessagesListLater(true);
			}));
		}

		public void ClearMessagesList()
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_messagesListVM.Items.Clear();
				_taskbarIcon.CloseBalloon();
			}));
		}

		public void KeepAlivePermanently()
		{
			// Sync across threads by invoking it on the dispatcher
			_myDispatcher.BeginInvoke(DispatcherPriority.Normal, (Action)(() =>
			{
				_messageListAliveUntil = DateTime.MaxValue;
			}));
		}

		public void FadeOutOrBasicallyDoWhatYouWantIDontCareAnymore()
		{
			CloseMessagesListLater(true);
		}
	}
}

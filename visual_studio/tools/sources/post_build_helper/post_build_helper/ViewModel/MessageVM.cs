using CgbPostBuildHelper.Model;
using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;

namespace CgbPostBuildHelper.ViewModel
{

	class MessageVM : BindableBase
	{
		private readonly Func<CgbAppInstanceVM> _instanceGetter;
		private readonly InvocationParams _config;
		private readonly Func<string> _fallbackInstNameGetter;
		private Model.Message _model;

		public MessageVM(CgbAppInstanceVM instance, Model.Message model)
		{
			_config = instance?.Config;
			_instanceGetter = () => instance;
			_fallbackInstNameGetter = () => instance?.ShortPath ?? "?";
			_model = model;
		}

		public MessageVM(IList<CgbAppInstanceVM> listOfInstances, InvocationParams config,  Model.Message model)
		{
			_config = config;
			_instanceGetter = () => listOfInstances.GetInstance(_config.ExecutablePath);
			_fallbackInstNameGetter = () => new FileInfo(_config.ExecutablePath).Name;
			_model = model;
		}

		public DateTime CreateDate => _model.CreateDate;

		public CgbAppInstanceVM AppInstance => _instanceGetter();

		public string AppInstancePath => _instanceGetter()?.Path ?? "";

		public string AppInstanceName => string.IsNullOrEmpty(AppInstancePath) ? _fallbackInstNameGetter() : new FileInfo(AppInstancePath).Name;

		public Brush MessageColor
		{
			get
			{
				switch (_model.MessageType)
				{
					case MessageType.Success:
						return View.Constants.SuccessBrushLight;
					case MessageType.Information:
						return View.Constants.InfoBrushLight;
					case MessageType.Warning:
						return View.Constants.WarningBrushLight;
					case MessageType.Error:
						return View.Constants.ErrorBrushLight;
					default:
						return View.Constants.InfoBrushLight;
				}
			}
		}

		public Brush AccentColor
		{
			get
			{
				switch (_model.MessageType)
				{
					case MessageType.Success:
						return View.Constants.SuccessBrushDark;
					case MessageType.Information:
						return View.Constants.InfoBrushDark;
					case MessageType.Warning:
						return View.Constants.WarningBrushDark;
					case MessageType.Error:
						return View.Constants.ErrorBrushDark;
					default:
						return View.Constants.InfoBrushDark;
				}
			}
		}

		public string MessageText
		{
			get => _model.MessageText;
			set => SetProperty(_model, m => m.MessageText, value);
		}

		public MessageType MessageType
		{
			get => _model.MessageType;
			set 
			{
				SetProperty(_model, m => m.MessageType, value);
				OnPropertyChanged("MessageTypeDescription");
				OnPropertyChanged("MessageTypeShortDescription");
			}
		}

		public string MessageTypeDescription
		{
			get
			{
				switch (_model.MessageType)
				{
					case MessageType.Success:
						return "Operation Succeeded";
					case MessageType.Information:
						return "Information";
					case MessageType.Warning:
						return "Warning";
					case MessageType.Error:
						return "Operation Failed/Error";
					default:
						return "???????";
				}
			}
		}

		public string MessageTypeShortDescription
		{
			get
			{
				switch (_model.MessageType)
				{
					case MessageType.Success:
						return "DONE";
					case MessageType.Information:
						return "INFO";
					case MessageType.Warning:
						return "WARN";
					case MessageType.Error:
						return "ERROR";
					default:
						return "????";
				}
			}
		}

		public ICommand AdditionalInfoCmd
		{
			get => _model.Action == null ? null : new DelegateCommand(_ => _model.Action());
		}

		public ICommand OpenInVisualStudio
		{
			get
			{
				if (null != _model.FilenameForFileActions && _model.FileCanBeEditedInVisualStudio)
				{
					return new DelegateCommand(_ =>
					{
						// Open file and goto line
						try
						{
							if (!VsUtils.ActivateFileInRunningVisualStudioInstances(_model.FilenameForFileActions, _model.LineNumberInFile))
							{
								if (!VsUtils.OpenFileInSpecificVisualStudioInstance(_config.VcxprojPath, _model.FilenameForFileActions, _model.LineNumberInFile))
								{
									if (!VsUtils.OpenFileInExistingVisualStudioInstance(_model.FilenameForFileActions, _model.LineNumberInFile))
									{
										string argument = _model.FilenameForFileActions;
										System.Diagnostics.Process.Start("devenv.exe", argument);
										VsUtils.OpenFileInExistingVisualStudioInstance(_model.FilenameForFileActions, _model.LineNumberInFile);
									}
								}
							}
						}
						catch (Exception e)
						{
							Console.Write(e.Message);
						}
					});
				}
				else
				{
					return null;
				}
			}
		}

		public ICommand OpenFile
		{
			get
			{
				if (null != _model.FilenameForFileActions)
				{
					return new DelegateCommand(_ =>
					{
						CgbUtils.OpenFileWithSystemViewer(_model.FilenameForFileActions);
					});
				}
				else
				{
					return null;
				}
			}
		}

		public ICommand ShowFileInDirectory
		{
			get
			{
				if (null != _model.FilenameForFileActions)
				{
					return new DelegateCommand(_ =>
					{
						CgbUtils.ShowDirectoryInExplorer(_model.FilenameForFileActions);
					});
				}
				else
				{
					return null;
				}
			}
		}

		public ICommand ShowInstanceDetailsCmd
		{
			get
			{
				var inst = _instanceGetter();
				if (null == inst)
				{
					return null;
				}
				return new DelegateCommand(_ =>
				{ 
					var window = new View.WindowToTheTop
					{
						Width = 800, Height = 600,
						Title = $"All events for {inst.ShortPath}"
					};
					window.InnerContent.Content = new View.EventFilesView()
					{
						DataContext = inst
					};
					window.Show();
				});
			}
		}
	}
}

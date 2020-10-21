using CgbPostBuildHelper.Model;
using CgbPostBuildHelper.Utils;
using CgbPostBuildHelper.ViewModel;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;

namespace CgbPostBuildHelper.ViewModel
{
	/// <summary>
	/// Contains all the data for one cgb-app-instance.
	/// </summary>
	class CgbAppInstanceVM : BindableBase
	{
		#region private members
		private InvocationParams _config;
		private readonly Dispatcher _myDispatcher;
		#endregion

		public CgbAppInstanceVM()
		{
			_myDispatcher = Dispatcher.CurrentDispatcher;

			Files.CollectionChanged += Files_CollectionChanged;
			AllEventsEver.CollectionChanged += AllEventsEver_CollectionChanged;
			CurrentlyWatchedDirectories.CollectionChanged += CurrentlyWatchedFiles_CollectionChanged;

			OpenEventDetails = new DelegateCommand(_ =>
			{
				var window = new View.WindowToTheTop
				{
					Width = 800, Height = 600,
					Title = $"All events for {ShortPath}"
				};
				window.InnerContent.Content = new View.EventFilesView()
				{
					DataContext = this
				};
				window.Show();
			});
		}

		private void CurrentlyWatchedFiles_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
		{
			OnPropertyChanged("CurrentlyWatchedFilesCount");	
		}

		private void AllEventsEver_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
		{
			OnPropertyChanged("LatestUpdate");
		}

		private void Files_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
		{
			OnPropertyChanged("FilesCount");
		}

		/// <summary>
		/// The (unique) identifier of an application instance.
		/// This is the path of the executable. 
		/// Since the path must be unique on the file system, the identifier is unique as well.
		/// </summary>
		public string Path => Config.ExecutablePath;
		
		/// <summary>
		/// The last part of Path, i.e. the "most significant bits" of the Path
		/// </summary>
		public string ShortPath
		{
			get
			{
				var fi = new FileInfo(Path);
				return System.IO.Path.Combine(fi.Directory?.Name ?? "", fi.Name);
			}
		}

		/// <summary>
		/// The parameters which have been passed from VisualStudio's custom build step
		/// </summary>
		public InvocationParams Config 
		{ 
			get => _config;
			set
			{
				SetProperty(ref _config, value);
				OnPropertyChanged("Path");
				OnPropertyChanged("ShortPath");
			}
		}

		/// <summary>
		/// List of all files which are managed by this app-instance
		/// </summary>
		public ObservableCollection<FileDeploymentDataVM> Files { get; } = new ObservableCollection<FileDeploymentDataVM>();

		/// <summary>
		/// Mapping of 'Dependency'-type paths to Files
		/// </summary>
		public Dictionary<string, List<FileDeploymentDataVM>> DependencyMapping { get; } = new Dictionary<string, List<FileDeploymentDataVM>>();

		/// <summary>
		/// Number of entries in the Files list
		/// </summary>
		public int FilesCount => Files.Count;

		/// <summary>
		/// A list of all build/update events that ever happened for this instance
		/// </summary>
		public ObservableCollection<CgbEventVM> AllEventsEver { get; } = new ObservableCollection<CgbEventVM>();

				
		/// <summary>
		/// Returns the date of the latest event/update (or null if there is none)
		/// </summary>
		public DateTime? LatestUpdate
		{
			get
			{
				if (AllEventsEver.Count == 0)
				{
					return null;
				}

				return (from x in AllEventsEver 
						orderby x.CreateDate descending 
						select x.CreateDate).First();
			}
		}

		public ICommand OpenEventDetails { get; }

		/// <summary>
		/// List of all files which are CURRENTLY being watched/monitored by a filesystem-watcher for changes.
		/// Typically, each FileDetailsVM instance will point to one of the entries inside the Files-collection.
		/// If it doesn't, this should actually mean that there is a bug.
		/// </summary>
		public ObservableCollection<WatchedDirectoryVM> CurrentlyWatchedDirectories { get; } = new ObservableCollection<WatchedDirectoryVM>();

		/// <summary>
		/// Number of files which are being watched currently
		/// </summary>
		public int CurrentlyWatchedFilesCount => CurrentlyWatchedDirectories.Count;

	}
}

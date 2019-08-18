using CgbPostBuildHelper.Model;
using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Media;

namespace CgbPostBuildHelper.ViewModel
{
	/// <summary>
	/// What's the type/origin of a certain event?
	/// </summary>
	enum CgbEventType
	{
		/// <summary>
		/// Event originates from a (post-)build step invocation
		/// </summary>
		Build,
		/// <summary>
		/// Event originates from a file-update (like file watcher having noticed a change)
		/// </summary>
		Update
	}

	/// <summary>
	/// Data to a certain event. In most cases, this will contain some files 
	/// (except when there are no assets or shaders assigned to a project at all)
	/// For events with the type 'Update', there will usually be exactly one file assigned.
	/// </summary>
	class CgbEventVM : BindableBase
	{
		private readonly DateTime _createDate = DateTime.Now;
		private readonly CgbEventType _eventType;

		public CgbEventVM(CgbEventType type)
		{
			_eventType = type;
			_filteredFiles = new ListCollectionView(Files);

			this.PropertyChanged += CgbEventVM_PropertyChanged;
		}

		private void CgbEventVM_PropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
		{
			FilesFiltered.Filter = delegate (object o)
			{
				var file = o as FileDeploymentDataVM;
				var ret = true;
				if (FilterErrors || FilterWarnings || FilterInfos || FilterSuccesses)
				{
					ret = false; // change default in this case
					if (FilterErrors && file.Messages.ContainsMessagesOfType(MessageType.Error)) ret = true;
					if (FilterWarnings && file.Messages.ContainsMessagesOfType(MessageType.Warning)) ret = true;
					if (FilterInfos && file.Messages.ContainsMessagesOfType(MessageType.Information)) ret = true;
					if (FilterSuccesses && file.Messages.ContainsMessagesOfType(MessageType.Success)) ret = true;
				}
				if (!string.IsNullOrWhiteSpace(FilterText))
				{ 
					ret = ret && (
						   file.InputFilePath.ToLowerInvariant().Contains(FilterText)
						|| file.OutputFilePath.ToLowerInvariant().Contains(FilterText)
						|| file.FilterPath.ToLowerInvariant().Contains(FilterText)
						|| file.FileTypeDescription.ToLowerInvariant().Contains(FilterText)
						|| file.DeploymentTypeDescription.ToLowerInvariant().Contains(FilterText)
					);
				}
				return ret;
			};
		}

		public DateTime CreateDate => _createDate;

		public CgbEventType Type => _eventType;

		private ListCollectionView _filteredFiles;
		private string _filterText = string.Empty;
		private bool _filterErrors = false;
		private bool _filterWarnings = false;
		private bool _filterInfos = false;
		private bool _filterSuccesses = false;

		public string FilterText 
		{ 
			get => _filterText;
			set => SetProperty(ref _filterText, value);
		}

		public bool FilterErrors
		{
			get => _filterErrors;
			set => SetProperty(ref _filterErrors, value);
		}
		public bool FilterWarnings
		{
			get => _filterWarnings;
			set => SetProperty(ref _filterWarnings, value);
		}
		public bool FilterInfos
		{
			get => _filterInfos;
			set => SetProperty(ref _filterInfos, value);
		}
		public bool FilterSuccesses
		{
			get => _filterSuccesses;
			set => SetProperty(ref _filterSuccesses, value);
		}


		public string TypeDescription
		{
			get
			{
				switch(_eventType)
				{
					case CgbEventType.Build:
						return "Buid-Event";
					case CgbEventType.Update:
						return "Update-Event";
				}
				return "?Unknown-Event?";
			}
		}

		public Brush EventColor
		{
			get
			{
				switch (_eventType)
				{
					case CgbEventType.Build:
						{
							if ((from x in Files select x.Messages.ContainsMessagesOfType(MessageType.Error)).Any(x => x == true))
							{
								return View.Constants.ErrorBrushDark;
							}
							if ((from x in Files select x.Messages.ContainsMessagesOfType(MessageType.Warning)).Any(x => x == true))
							{
								return View.Constants.WarningBrushDark;
							}
							return View.Constants.SuccessBrushDark;
						}
					case CgbEventType.Update:
						{
							if ((from x in Files select x.Messages.ContainsMessagesOfType(MessageType.Error)).Any(x => x == true))
							{
								return View.Constants.ErrorBrushLight;
							}
							if ((from x in Files select x.Messages.ContainsMessagesOfType(MessageType.Warning)).Any(x => x == true))
							{
								return View.Constants.WarningBrushLight;
							}
							return View.Constants.SuccessBrushLight;
						}
				}
				return View.Constants.ErrorBrushDark; // That should not happen
			}
		}

		public ObservableCollection<FileDeploymentDataVM> Files { get; } = new ObservableCollection<FileDeploymentDataVM>();

		public ListCollectionView FilesFiltered => _filteredFiles;

	}
}

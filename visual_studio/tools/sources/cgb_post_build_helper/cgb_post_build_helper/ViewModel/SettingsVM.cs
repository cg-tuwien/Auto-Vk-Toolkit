using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CgbPostBuildHelper.ViewModel
{
	class SettingsVM : BindableBase
	{
		private readonly WpfApplication _application;

		public SettingsVM(WpfApplication application)
		{
			_application = application;
		}

		public bool AlwaysDeployReleaseDlls
		{
			get
			{
				return Properties.Settings.Default.AlwaysDeployReleaseDlls;
			}
			set
			{
				Properties.Settings.Default.AlwaysDeployReleaseDlls = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public bool CopyAssetsToOutputDirectory
		{
			get
			{
				return Properties.Settings.Default.AlwaysCopyNeverSymlink;
			}
			set
			{
				Properties.Settings.Default.AlwaysCopyNeverSymlink = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public bool HideAccessDeniedForDlls
		{
			get
			{
				return Properties.Settings.Default.HideAccessDeniedErrorsForDlls;
			}
			set
			{
				Properties.Settings.Default.HideAccessDeniedErrorsForDlls = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public bool DoNotMonitorFiles
		{
			get
			{
				return Properties.Settings.Default.DoNotMonitorFiles;
			}
			set
			{
				Properties.Settings.Default.DoNotMonitorFiles = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();

				if (!Properties.Settings.Default.DoNotMonitorFiles)
				{
					_application.EndAllWatches();
				}
			}
		}

		public bool ShowWindowForVulkanShaders
		{
			get
			{
				return Properties.Settings.Default.ShowWindowForVkShaderDeployment;
			}
			set
			{
				Properties.Settings.Default.ShowWindowForVkShaderDeployment = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public bool ShowWindowForGlShaders
		{
			get
			{
				return Properties.Settings.Default.ShowWindowForGlShaderDeployment;
			}
			set
			{
				Properties.Settings.Default.ShowWindowForGlShaderDeployment = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public bool ShowWindowForModels
		{
			get
			{
				return Properties.Settings.Default.ShowWindowForModelDeployment;
			}
			set
			{
				Properties.Settings.Default.ShowWindowForModelDeployment = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public string ExternalsReleaseSubPath
		{
			get
			{
				return Properties.Settings.Default.ReleaseSubPathInExternals;
			}
			set
			{
				Properties.Settings.Default.ReleaseSubPathInExternals = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}

		public string ExternalsDebugSubPath
		{
			get
			{
				return Properties.Settings.Default.DebugSubPathInExternals;
			}
			set
			{
				Properties.Settings.Default.DebugSubPathInExternals = value;
				Properties.Settings.Default.Save();
				IssueOnPropertyChanged();
			}
		}


	}
}

using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

namespace CgbPostBuildHelper.ViewModel
{
	class ContextMenuActionsVM : BindableBase
	{
		private WpfApplication _application;

		public ContextMenuActionsVM(WpfApplication application)
		{
			_application = application;

			ShowInstances = new DelegateCommand(_ =>
			{
				var wnd = new View.InstancesList
				{
					DataContext = new { Items =  _application.AllInstances }
				};
				wnd.Show();
			});

			ShowMessages = new DelegateCommand(_ =>
			{
				_application.ShowMessagesList();
			});

			ClearMessages = new DelegateCommand(_ =>
			{
				_application.ClearMessagesList();
			});

			OpenSettings = new DelegateCommand(_ =>
			{
				var wnd = new View.SettingsView
				{
					DataContext = new ViewModel.SettingsVM(_application)
				};
				wnd.Show();
			});

			OpenAbout = new DelegateCommand(_ =>
			{
				var wnd = new View.AboutView
				{
					Title = "About: Post Build Helper"
				};
				wnd.Show();
			});

			ExitApplicationCommand = new DelegateCommand(_ =>
			{
				_application.EndAllWatchesAndExitApplication();
			});
		}

		public ICommand ShowInstances { get; set; }
		
		public ICommand ShowMessages { get; set; }

		public ICommand ClearMessages { get; set; }

		public ICommand OpenSettings { get; set; }

		public ICommand OpenAbout { get; set; }

		public ICommand ExitApplicationCommand { get; set; }
	}
}

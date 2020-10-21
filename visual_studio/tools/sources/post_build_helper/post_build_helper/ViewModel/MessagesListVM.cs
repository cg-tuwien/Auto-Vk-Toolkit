using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace CgbPostBuildHelper.ViewModel
{
	class MessagesListVM : BindableBase
	{   
		private readonly WpfApplication _app;

		public MessagesListVM(WpfApplication app)
		{
			_app = app;
		}

		public ObservableCollection<MessageVM> Items { get; } = new ObservableCollection<MessageVM>();

		public ICommand DismissCommand
		{
			get => new DelegateCommand(_ =>
			{
				_app.CloseMessagesListNow();
			});
		}
	}
}

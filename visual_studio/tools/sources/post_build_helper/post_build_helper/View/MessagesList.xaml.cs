using CgbPostBuildHelper.ViewModel;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CgbPostBuildHelper.View
{
	/// <summary>
	/// Interaction logic for MessagesList.xaml
	/// </summary>
	public partial class MessagesList : UserControl
	{
		private IMessageListLifetimeHandler _lifetimeHandler;

		public IMessageListLifetimeHandler LifetimeHandler
		{
			get => _lifetimeHandler;
			set
			{
				this.MouseEnter -= MessagesList_MouseEnter;
				this.MouseLeave -= MessagesList_MouseLeave;
				_lifetimeHandler = value;
				this.MouseEnter += MessagesList_MouseEnter;
				this.MouseLeave += MessagesList_MouseLeave;
			}
		}

		private void MessagesList_MouseEnter(object sender, MouseEventArgs e)
		{
			_lifetimeHandler.KeepAlivePermanently();
		}

		private void MessagesList_MouseLeave(object sender, MouseEventArgs e)
		{
			_lifetimeHandler.FadeOutOrBasicallyDoWhatYouWantIDontCareAnymore();
		}


		public MessagesList()
		{
			InitializeComponent();
			
		}
	}
}

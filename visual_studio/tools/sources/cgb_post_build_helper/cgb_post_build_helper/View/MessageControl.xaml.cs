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
	/// Interaction logic for MessageControl.xaml
	/// </summary>
	public partial class MessageControl : UserControl
	{
		public MessageControl()
		{
			InitializeComponent();
			this.MouseEnter += MessageControl_MouseEnter;
			this.MouseLeave += MessageControl_MouseLeave;
		}

		private void MessageControl_MouseEnter(object sender, MouseEventArgs e)
		{
			theBorder.BorderThickness = new Thickness(3.0);
			e.Handled = false;
		}

		private void MessageControl_MouseLeave(object sender, MouseEventArgs e)
		{
			theBorder.BorderThickness = new Thickness(2.0);
			e.Handled = false;
		}
	}
}

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
using System.Windows.Shapes;

namespace CgbPostBuildHelper.View
{
	/// <summary>
	/// Interaction logic for WindowToTheTop.xaml
	/// </summary>
	public partial class WindowToTheTop : Window
	{
		public WindowToTheTop()
		{
			this.Initialized += WindowToTheTop_Initialized;
			this.ContentRendered += WindowToTheTop_ContentRendered;
			this.PreviewKeyUp += WindowToTheTop_PreviewKeyUp; ;
			InitializeComponent();
		}

		private void WindowToTheTop_PreviewKeyUp(object sender, KeyEventArgs e)
		{
			if (e.Key == Key.T) // Maybe toggle
			{
				if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
				{
					this.Topmost = !this.Topmost;
				}
			}
			else if (e.Key == Key.W) // Maybe close
			{
				if (Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl))
				{
					this.Close();
				}
			}
		}

		private void WindowToTheTop_Initialized(object sender, EventArgs e)
		{
			this.Initialized -= WindowToTheTop_Initialized;

			if (!this.IsVisible)
			{
				this.Show();
			}

			if (this.WindowState == WindowState.Minimized)
			{
				this.WindowState = WindowState.Normal;
			}

			this.Activate();
			this.Topmost = true;
		}

		private void WindowToTheTop_ContentRendered(object sender, EventArgs e)
		{
			this.ContentRendered -= WindowToTheTop_ContentRendered;
			this.Topmost = false;
			this.Focus();
		}

		private void Button_Click(object sender, RoutedEventArgs e)
		{
			this.Topmost = !this.Topmost;
		}
	}
}

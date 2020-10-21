using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;

namespace CgbPostBuildHelper.View
{
	static class Constants
	{
		public static readonly string DateTimeUiFormat = System.Threading.Thread.CurrentThread.CurrentUICulture.DateTimeFormat.FullDateTimePattern;

		public static readonly Brush ErrorBrushDark;
		public static readonly Brush ErrorBrushLight;

		public static readonly Brush WarningBrushDark;
		public static readonly Brush WarningBrushLight;

		public static readonly Brush InfoBrushDark;
		public static readonly Brush InfoBrushLight;

		public static readonly Brush SuccessBrushDark;
		public static readonly Brush SuccessBrushLight;

		static Constants()
		{
			//The SolidColorBrush is a Freezable which is a derived DispatcherObject.DispatcherObjects have thread affinity -i.e it 
			//can only be used / interacted with on the thread on which it was created.Freezables however do offer the ability to freeze 
			//an instance. This will prevent any further changes to the object but it will also release the thread affinity.So you can 
			//either change it so that your property is not storing a DependencyObject like SolidColorBrush and instead just store the 
			//Color. Or you can freeze the SolidColorBrush that you are creating using the Freeze method.

			ErrorBrushDark = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#ff1a66");
			ErrorBrushDark.Freeze(); // => make available on other threads
			ErrorBrushLight = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#ffccdd");
			ErrorBrushLight.Freeze(); // => make available on other threads

			WarningBrushDark = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#e64d00");
			WarningBrushDark.Freeze(); // => make available on other threads
			WarningBrushLight = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#ffddcc");
			WarningBrushLight.Freeze(); // => make available on other threads

			InfoBrushDark = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#0099cc");
			InfoBrushDark.Freeze(); // => make available on other threads
			InfoBrushLight = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#ccf2ff");
			InfoBrushLight.Freeze(); // => make available on other threads

			SuccessBrushDark = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#2d8659");
			SuccessBrushDark.Freeze(); // => make available on other threads
			SuccessBrushLight = (Brush)new System.Windows.Media.BrushConverter().ConvertFromInvariantString("#d9f2e6");
			SuccessBrushLight.Freeze(); // => make available on other threads
		}
	}
}

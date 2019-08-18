using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace CgbPostBuildHelper.Converter
{
	class NullToVisibilityConverter : IValueConverter
	{
		public Visibility CaseNull { get; set; } = Visibility.Collapsed;
		public Visibility CaseNonNull { get; set; } = Visibility.Visible;

		public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
		{
			return null == value ? CaseNull : CaseNonNull;
		}

		public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
		{
			throw new NotImplementedException();
		}
	}
}

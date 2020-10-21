using CgbPostBuildHelper.Utils;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace CgbPostBuildHelper.ViewModel
{
	class FileVM : BindableBase
	{
		public ICommand OpenFileCommand
		{
			get => new DelegateCommand(path =>
			{
				CgbUtils.OpenFileWithSystemViewer((string)path);
			});
		}

		public ICommand OpenFolderCommand
		{
			get => new DelegateCommand(path =>
			{
				CgbUtils.ShowDirectoryInExplorer((string)path);
			});
		}
	}
}

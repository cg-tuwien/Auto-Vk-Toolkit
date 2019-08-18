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
	class WatchedFileVM : FileVM
	{
		public WatchedFileVM(string filterPath, string filePath)
		{
			FilterPath = filterPath;
			Path = filePath;
		}

		public string FilterPath { get; }

		public string Path { get; }
		
		public string FileName
		{
			get
			{
				return new FileInfo(Path).Name;
			}
		}

		public string SomewhatShortPath
		{
			get
			{
				var fi = new FileInfo(Path);
				return System.IO.Path.Combine(
					fi.Directory?.Parent?.Parent?.Name ?? "",
					fi.Directory?.Parent?.Name ?? "",
					fi.Directory?.Name ?? "",
					fi.Name);
			}
		}
	}
}

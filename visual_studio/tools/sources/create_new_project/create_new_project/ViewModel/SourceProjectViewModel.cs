using CreateNewProject.Utils;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CreateNewProject.ViewModel
{
	class SourceProjectViewModel
	{
		private FileInfo _projInfo;
		private FileInfo _slnInfo;

		public SourceProjectViewModel(string slnPath, string projPath)
		{
			_slnInfo = null == slnPath ? null : new FileInfo(slnPath);
			_projInfo = null == projPath ? null : new FileInfo(projPath);
		}

		public string FullProjPath => null == _projInfo ? "?" : _projInfo.FullName;

		public string ProjFileName => null == _projInfo ? "?" : _projInfo.Name;

		public string PathRelativeToSln => null == _slnInfo ? "?" : PathNetCore.GetRelativePath(_slnInfo.DirectoryName, FullProjPath);
	}
}

using CgbPostBuildHelper.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Text.RegularExpressions;
using Diag = System.Diagnostics;
using System.Security.Cryptography;
using CgbPostBuildHelper.ViewModel;
using Assimp;
using System.Runtime.InteropServices;

namespace CgbPostBuildHelper.Deployers
{
	class CopyFileDeployment : DeploymentBase
	{
		public override void Deploy()
		{
			var outPath = new FileInfo(_outputFilePath);
			Directory.CreateDirectory(outPath.DirectoryName);

			var assetFileModel = PrepareNewAssetFile(null);
			assetFileModel.FileType = FileType.Generic;
			assetFileModel.OutputFilePath = outPath.FullName;

			// Now, are we going to copy or are we going to symlink?
			FilesDeployed.Add(assetFileModel); // store before deployment to keep the info - even if DeployFile() might fail
			DeployFile(assetFileModel);
		}

	}


}

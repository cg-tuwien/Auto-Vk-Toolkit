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
using CgbPostBuildHelper.Utils;

namespace CgbPostBuildHelper.Deployers
{

	class ObjModelDeployment : ModelDeployment
	{
		public override void Deploy()
		{
			base.Deploy();

			string matFile = null;
			using (var sr = new StreamReader(_inputFile.FullName))
			{
				while (!sr.EndOfStream)
				{
					var line = sr.ReadLine();
					if (line.TrimStart().StartsWith("mtllib"))
					{
						// found a .mat-file!
						matFile = line.TrimStart().Substring("mtllib".Length).Trim();
					}
				}
			}

			if (null != matFile)
			{
                var matInPathStr = Path.Combine(_inputFile.DirectoryName, matFile);
                var matInPath = new FileInfo(matInPathStr);

                // Perform some checks before deploying it
                // -> A dependent resource must reside in the same folder or in a subfolder of the root resource.
                bool failed = false;
                if (!matInPath.Directory.IsSameOrSubdirectoryOf(_inputFile.Directory))
                {
                    // The first element of FilesDeployed will contain the 3D-model file
                    FilesDeployed.First().Messages.Add(Message.Create(MessageType.Warning, $"The material file '{matInPath.FullName}' is not located in the same directory or a subdirectory of {_inputFile.FullName}. It will not be deployed.", null));
                    failed = true;
                }
                if (!matInPath.Exists)
                {
                    // The first element of FilesDeployed will contain the 3D-model file
                    FilesDeployed.First().Messages.Add(Message.Create(MessageType.Warning, $"The material file '{matInPath.FullName}' (referenced in '{_inputFile.Name}') does not exist at that path.", null));
                    failed = true;
                }

                if (!failed)
                {
                    var modelOutPath = new FileInfo(_outputFilePath);
				    var actualMatPath = Path.Combine(modelOutPath.DirectoryName, matFile);
				    var matOutPath = new FileInfo(actualMatPath);
				    Directory.CreateDirectory(matOutPath.DirectoryName);

				    Diag.Debug.Assert(FilesDeployed[0].FileType == FileType.Generic3dModel);
				    Diag.Debug.Assert(FilesDeployed[0].Parent == null);
				    var assetFileMat = PrepareNewAssetFile(FilesDeployed[0]);
				    // Alter input path:
				    assetFileMat.InputFilePath = matInPathStr;
				    assetFileMat.FileType = FileType.ObjMaterials;
				    assetFileMat.OutputFilePath = matOutPath.FullName;
				    DeployFile(assetFileMat);

				    assetFileMat.Messages.Add(Message.Create(MessageType.Success, $"Added materials file '{assetFileMat.OutputFilePath}', of .obj model '{FilesDeployed[0].OutputFilePath}'", null)); // TODO: open a window or so?

				    FilesDeployed.Add(assetFileMat);
                }
			}
		}
	}
}

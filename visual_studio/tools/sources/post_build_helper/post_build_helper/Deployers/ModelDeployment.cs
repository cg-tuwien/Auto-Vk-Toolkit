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
	class TextureCleanupData
	{
		public string FullOutputPathNormalized { get; set; }
		public string FullInputPathNormalized { get; set; }
		public string OriginalRelativePath { get; set; }
	}

	static class TextureCleanupDataHelpers
	{
		public static TextureCleanupData FindFullOutputPath(this IList<TextureCleanupData> collection, string path)
		{
			path = CgbUtils.NormalizePath(path);
			var existing = (from x in collection where x.FullOutputPathNormalized == path select x).FirstOrDefault();
			return existing;
		}
	}

	class ModelDeployment : DeploymentBase
	{

		// Use a List, because order matters (maybe)
		List<TextureCleanupData> _outputToInputPathsNormalized;

		private string GetInputPathForTexture(string textureRelativePath)
		{
			return Path.Combine(_inputFile.DirectoryName, textureRelativePath);
		}

		private string GetOutputPathForTexture(string textureRelativePath)
		{
			return Path.Combine(new FileInfo(_outputFilePath).DirectoryName, textureRelativePath);
		}

		private void RebuildOutputToInputPathRecords()
		{
			_outputToInputPathsNormalized = new List<TextureCleanupData>();
			foreach (var txp in _texturePaths)
			{
				var outPath = GetOutputPathForTexture(txp);
				var outPathNrm = CgbUtils.NormalizePath(outPath);
				var existing = _outputToInputPathsNormalized.FindFullOutputPath(outPathNrm);
				if (null == existing)
				{
					var inpPath = GetInputPathForTexture(txp);
					var inpPathNrm = CgbUtils.NormalizePath(inpPath);
					_outputToInputPathsNormalized.Add(new TextureCleanupData
					{
						FullOutputPathNormalized = outPathNrm,
						FullInputPathNormalized = inpPathNrm,
						OriginalRelativePath = txp
					});
				}
			}
		}

		/// <summary>
		/// Set textures which are referenced by the model for it's materials.
		/// Those textures are to be deployed as well.
		/// </summary>
		/// <param name="textures">Assigned textures which shall be deployed</param>
		public void SetTextures(IEnumerable<string> textures)
		{
			if (_outputFilePath == null )
			{
				throw new InvalidOperationException("Call this method AFTER you have initialized the deployment via SetInputParameters!");
			}

			_texturePaths.AddRange(textures);
			RebuildOutputToInputPathRecords();
		}

		/// <summary>
		/// Determines if this deployment has conflicts with an other deployment,
		/// i.e. same output paths, but different input paths.
		/// (If such a case is detected, the conflict should be solved before deployment)
		/// </summary>
		/// <param name="other">The other deployment to compare with</param>
		/// <returns>true if there is a conflict, false otherwise</returns>
		public override bool HasConflictWith(DeploymentBase other)
		{
			if (other is ModelDeployment otherModel)
			{
				System.Diagnostics.Debug.Assert(null != this._outputToInputPathsNormalized); // If that assert fails, SetTextures has not been invoked before
				System.Diagnostics.Debug.Assert(null != otherModel._outputToInputPathsNormalized); // If that assert fails, SetTextures has not been invoked before
				// Process in order, to (hopefully) maintain the original order.
				// The purpose of this shall be to have consistent behavior across multiple events.
				foreach (var pathRecord in _outputToInputPathsNormalized)
				{
					var existingInOther = otherModel._outputToInputPathsNormalized.FindFullOutputPath(pathRecord.FullOutputPathNormalized);
					if (null != existingInOther)
					{
						if (pathRecord.FullInputPathNormalized != existingInOther.FullInputPathNormalized) // found a conflict! Input paths do not match!
							return true;
					}
				}
			}
			// In any case, perform the check of the base class:
			return base.HasConflictWith(other);
		}

		/// <summary>
		/// No use in deploying the same file multiple times!
		/// Therefore, use this method to cleanup repeating paths across multiple models.
		/// </summary>
		public void KickOutAllTexturesWhichAreAvailableInOther(ModelDeployment other)
		{
			foreach (var pathRecord in _outputToInputPathsNormalized)
			{
				if (null != other._outputToInputPathsNormalized.FindFullOutputPath(pathRecord.FullOutputPathNormalized))
				{
					// okay, found it! => Kick it!
					_texturePaths.Remove(pathRecord.OriginalRelativePath);
				}
			}
			// List of textures possibly changed!
			RebuildOutputToInputPathRecords();
		}

		public void ResolveConflictByMovingThisToSubfolder()
		{
			// Check if we haven't already!
			var curOutput = new FileInfo(_outputFilePath);
			if (curOutput.Name == curOutput.Directory.Name)
			{
				throw new Exception($"It looks like this model has already been moved into a subfolder: current output path = '{_outputFilePath}'");
			}

			// Okay, let's move!
			_outputFilePath = Path.Combine(curOutput.Directory.FullName, curOutput.Name, curOutput.Name);
			// Also update all the new texture target paths:
			RebuildOutputToInputPathRecords();
		}

		public override void Deploy()
		{
			var modelOutPath = new FileInfo(_outputFilePath);
			Directory.CreateDirectory(modelOutPath.DirectoryName);

			var assetFileModel = PrepareNewAssetFile(null);
			assetFileModel.FileType = FileType.Generic3dModel;
			assetFileModel.OutputFilePath = modelOutPath.FullName;
			DeployFile(assetFileModel);
			FilesDeployed.Add(assetFileModel);

			foreach (var tp in _texturePaths)
			{
                var texInPathStr = GetInputPathForTexture(tp);
                var texInPath = new FileInfo(texInPathStr);

                // Perform some checks before deploying it
                // -> A dependent resource must reside in the same folder or in a subfolder of the root resource.
                if (!texInPath.Directory.IsSameOrSubdirectoryOf(_inputFile.Directory))
                {
                    assetFileModel.Messages.Add(Message.Create(MessageType.Warning, $"The texture '{texInPath.FullName}' is not located in the same directory or a subdirectory of {_inputFile.FullName}. It will not be deployed.", null));
                    continue;
                }
                if (!texInPath.Exists)
                {
                    assetFileModel.Messages.Add(Message.Create(MessageType.Warning, $"The texture '{texInPath.FullName}' (referenced in '{_inputFile.Name}') does not exist at that path.", null));
                    continue;
                }

                var texOutPathStr = GetOutputPathForTexture(tp);
				var texOutPath = new FileInfo(texOutPathStr);

				Directory.CreateDirectory(texOutPath.DirectoryName);

				var assetFileTex = PrepareNewAssetFile(assetFileModel);
				// Alter input path:
				assetFileTex.InputFilePath = texInPathStr;
				assetFileTex.FileType = FileType.Generic;
				assetFileTex.OutputFilePath = texOutPath.FullName;
				DeployFile(assetFileTex);
				FilesDeployed.Add(assetFileTex);
			}

			assetFileModel.Messages.Add(Message.Create(MessageType.Success, $"Copied model '{assetFileModel.OutputFilePath}', and {_texturePaths.Count} dependent material textures.", null)); // TODO: open a window or so?
		}

		protected readonly List<string> _texturePaths = new List<string>();
	}

}

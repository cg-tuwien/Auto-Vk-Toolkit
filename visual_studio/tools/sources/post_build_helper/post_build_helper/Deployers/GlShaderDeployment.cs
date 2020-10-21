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
using CgbPostBuildHelper.Model;
using Assimp;
using System.Runtime.InteropServices;

namespace CgbPostBuildHelper.Deployers
{
	class GlShaderDeployment : DeploymentBase
	{
		private static readonly Dictionary<string, string> GlslVkToGlReplacements = new Dictionary<string, string>()
		{
			{"gl_VertexIndex", "gl_VertexID"},
			{"gl_InstanceIndex", "gl_InstanceID"},
		};
		private static readonly Regex RegexGlslLayoutSetBinding = new Regex(@"(layout\s*\(.*)(set\s*\=\s*\d+\s*\,\s*)(binding\s*\=\s*\d+)(.*\))",
			RegexOptions.Compiled);
		private static readonly Regex RegexGlslLayoutSetBinding2 = new Regex(@"(layout\s*\(.*)(binding\s*\=\s*\d\s*)(\,\s*set\s*\=\s*\d+)(.*\))",
			RegexOptions.Compiled);

		public static string MorphVkGlslIntoGlGlsl(string vkGlsl)
		{
			string glGlsl = GlslVkToGlReplacements.Aggregate(vkGlsl, (current, pair) => current.Replace(pair.Key, pair.Value));
			glGlsl = RegexGlslLayoutSetBinding.Replace(glGlsl, match => match.Groups[1].Value + match.Groups[3].Value + match.Groups[4].Value);
			glGlsl = RegexGlslLayoutSetBinding2.Replace(glGlsl, match => match.Groups[1].Value + match.Groups[2].Value + match.Groups[4].Value);
			// TODO: There is still a lot of work to do
			// See also: https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt
			return glGlsl;
		}

		public override void Deploy()
		{
			var outFile = new FileInfo(_outputFilePath);
			Directory.CreateDirectory(outFile.DirectoryName);
			// Read in -> modify -> write out
			string glslCode = File.ReadAllText(_inputFile.FullName);
			File.WriteAllText(outFile.FullName, MorphVkGlslIntoGlGlsl(glslCode));

			var assetFile = PrepareNewAssetFile(null);
			assetFile.FileType = FileType.GlslShaderForGl;
			assetFile.OutputFilePath = outFile.FullName;
			assetFile.DeploymentType = DeploymentType.MorphedCopy;

			assetFile.Messages.Add(Message.Create(MessageType.Success, $"Copied (Vk->Gl morphed) GLSL file to '{outFile.FullName}'", null)); // TODO: open a window or so?

			FilesDeployed.Add(assetFile);
		}
	}

}

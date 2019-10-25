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
using CgbPostBuildHelper.Deployers;
using System.Diagnostics;
using Newtonsoft.Json;

namespace CgbPostBuildHelper.Utils
{
	static class CgbUtils
	{
		private static readonly Regex RegexIsInAssets = new Regex(@"^assets([\\\/].+)*$", 
			RegexOptions.Compiled | RegexOptions.IgnoreCase);
		private static readonly Regex RegexIsInShaders = new Regex(@"^shaders([\\\/].+)*$", 
			RegexOptions.Compiled | RegexOptions.IgnoreCase);

		private static readonly MD5 Md5Implementation = MD5.Create();
		
		/// <summary>
		/// Calculates the MD5 hash of a file
		/// </summary>
		/// <param name="filePath">Path to the file</param>
		/// <returns>The has as byte-array</returns>
		public static byte[] CalculateFileHash(string filePath)
		{
			byte[] hash;
			using (var inputStream = File.Open(filePath, FileMode.Open, FileAccess.Read, FileShare.Read))
			{
				hash = Md5Implementation.ComputeHash(inputStream);
			}
			return hash;
		}
		
		/// <summary>
		/// Determines if the two given hashes are equal
		/// </summary>
		/// <param name="hash1">First hash to compare with each other</param>
		/// <param name="hash2">Second hash to compare with each other</param>
		/// <returns>true if they are equal</returns>
		static bool AreHashesEqual(byte[] hash1, byte[] hash2)
		{
			return hash1.SequenceEqual(hash2);
		}

		public static string NormalizePartialPath(string path)
		{
			return path.Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar)
					   .Trim()
					   .Trim(Path.DirectorySeparatorChar)
					   .ToUpperInvariant();
		}

		public static string NormalizePath(string path)
		{
			return Path.GetFullPath(new Uri(path).LocalPath)
					   .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar)
					   .ToUpperInvariant();
		}

        /// <summary>
        /// Checks if a given directory is the same directory or a subdirectory of a given "root"-directory.
        /// </summary>
        /// <param name="inQuestion">The directory which should be checked if it is the same or a subdirectory of "root".</param>
        /// <param name="root">The root directory to compare with.</param>
        public static bool IsSameOrSubdirectoryOf(this DirectoryInfo inQuestion, DirectoryInfo root)
        {
            var rootFullPath = NormalizePath(root.FullName);
            while (inQuestion != null)
            {
                if (NormalizePath(inQuestion.FullName) == rootFullPath)
                {
                    return true;
                }
                else
                {
                    inQuestion = inQuestion.Parent;
                }
            }
            return false;
        }


		/// <summary>
		/// Helper function for extracting a value for a named argument out of the command line arguments.
		/// Names and values always come in pairs of the form: "-paramName C:\parameter_value"
		/// </summary>
		/// <param name="argName">Name of the argument we are looking for</param>
		/// <param name="args">All command line parameters</param>
		/// <returns>String containing the requested argument's associated value.</returns>
		private static string ExtractValueForNamedArgument(string argName, string[] args)
		{
			argName = argName.Trim();

			if (!argName.StartsWith("-"))
			{
				throw new ArgumentException($"'{argName}' is not a valid argument name; valid argument names start with '-'.");
			}

			for (int i=0; i < args.Length; ++i)
			{
				if (string.Compare(argName, args[i].Trim(), true) == 0)
				{
					if (args.Length == i + 1)
					{
						throw new ArgumentException($"There is no value after the argument named '{argName}'");
					}
					if (args[i+1].Trim().StartsWith("-"))
					{
						throw new ArgumentException($"Invalid value following argument name '{argName}' or value not present after it.");
					}
					// all good (hopefully)
					return args[i+1].Trim();
				}
			}

			throw new ArgumentException($"There is no argument named '{argName}'.");
		}

        /// <summary>
        /// Helper function for extracting a value for a named argument out of the command line arguments.
        /// Names and values always come in pairs of the form: "-paramName C:\parameter_value"
        /// </summary>
        /// <param name="argName">Name of the argument we are looking for</param>
        /// <param name="args">All command line parameters</param>
        /// <returns>String containing the requested argument's associated value.</returns>
        private static List<string> ExtractMultipleValuesForNamedArguments(string argName, string[] args)
        {
            argName = argName.Trim();

            if (!argName.StartsWith("-"))
            {
                throw new ArgumentException($"'{argName}' is not a valid argument name; valid argument names start with '-'.");
            }

            List<string> results = new List<string>();

            for (int i = 0; i < args.Length; ++i)
            {
                if (string.Compare(argName, args[i].Trim(), true) == 0)
                {
                    if (args.Length == i + 1)
                    {
                        throw new ArgumentException($"There is no value after the argument named '{argName}'");
                    }
                    if (args[i + 1].Trim().StartsWith("-"))
                    {
                        throw new ArgumentException($"Invalid value following argument name '{argName}' or value not present after it.");
                    }
                    // all good (hopefully)
                    results.Add(args[i + 1].Trim());
                }
            }

            if (results.Count > 0)
            {
                return results;
            }

            throw new ArgumentException($"There is no argument named '{argName}'.");
        }

        /// <summary>
        /// Parse command line arguments and put them into an InvocationParams struct, if parsing was successful.
        /// </summary>
        /// <param name="args">Command line arguments</param>
        /// <returns>A new instance of InvocationParams</returns>
        public static InvocationParams ParseCommandLineArgs(string[] args)
		{
			var configuration = ExtractValueForNamedArgument("-configuration", args).ToLowerInvariant();
			var p = new InvocationParams
			{
				CgbFrameworkPath = ExtractValueForNamedArgument("-framework", args),
				CgbExternalPaths = ExtractMultipleValuesForNamedArguments("-external", args),
				TargetApi = configuration.Contains("gl") 
							? BuildTargetApi.OpenGL 
							: configuration.Contains("vulkan")
							  ? BuildTargetApi.Vulkan
							  : throw new ArgumentException("Couldn't determine the build target API from the '-configuration' argument's value."),
				Configuration = configuration.Contains("debug")
								? BuildConfiguration.Debug
								: configuration.Contains("release")
								  ? BuildConfiguration.Release
								  : configuration.Contains("publish")
									? BuildConfiguration.Publish 
									: throw new ArgumentException("Couldn't determine the build configuration from the '-configuration' argument's value."),
				Platform = ExtractValueForNamedArgument("-platform", args).ToLower() == "x64" 
						   ? BuildPlatform.x64 
						   : throw new ArgumentException("Target platform does not seem to be 'x64'."),
				VcxprojPath = ExtractValueForNamedArgument("-vcxproj", args),
				FiltersPath = ExtractValueForNamedArgument("-filters", args),
				OutputPath = ExtractValueForNamedArgument("-output", args),
				ExecutablePath = ExtractValueForNamedArgument("-executable", args)
			};
			return p;
		}

		/// <summary>
		/// Finds a specific instance in the list of watched files. An watched file is uniquely identified by a path
		/// </summary>
		/// <param name="list">Search space</param>
		/// <param name="path">Key</param>
		/// <returns>A watched file entry with matching key or null</returns>
		public static WatchedFileVM GetFile(this IList<WatchedFileVM> list, string path) 
		{
			if (string.IsNullOrEmpty(path))
			{
				return null;
			}
			path = NormalizePath(path);
			return (from x in list where string.Compare(NormalizePath(x.Path), path, true) == 0 select x).FirstOrDefault();	
		}

		/// <summary>
		/// Finds a specific instance in the list of instances. An instance is uniquely identified by a path
		/// </summary>
		/// <param name="list">Search space</param>
		/// <param name="path">Key</param>
		/// <returns>An instance with matching key or null</returns>
		public static CgbAppInstanceVM GetInstance(this IList<CgbAppInstanceVM> list, string path)
		{
			if (string.IsNullOrEmpty(path))
			{
				return null;
			}
			path = NormalizePath(path);
			return (from x in list where string.Compare(NormalizePath(x.Path), path, true) == 0 select x).FirstOrDefault();
		}

		public static void PrepareDeployment(InvocationParams config, string filePath, string filterPath, out IFileDeployment outDeployment)
		{
			outDeployment = null;
			
			// Prepare for what there is to come:
			var inputFile = new FileInfo(filePath);
			if (!inputFile.Exists)
			{
				throw new FileNotFoundException($"File '{filePath}' does not exist");
			}
				
			// There are two special filter paths: "assets" and "shaders".
			// Files under both filter paths are copied to the output directory, but
			// only those under "shaders" are possibly compiled to SPIR-V (in case
			// of building against Vulkan) or their GLSL code might be modified (in
			// case of building against OpenGL)
			//
			// All in all, we have the following special cases:
			//  #1: If it is a shader file and we're building for Vulkan => compile to SPIR-V
			//  #2: If it is a shader file and we're building for OpenGL => modify GLSL
			//  #3: If it is an .obj 3D Model file => get its materials file

			bool isExternalDependency = false;
            foreach (var configCgbExternalPath in config.CgbExternalPaths)
            {
                if (CgbUtils.NormalizePath(inputFile.FullName).Contains(CgbUtils.NormalizePath(configCgbExternalPath)))
                {
                    isExternalDependency = true;
                }
            }
			var isAsset = RegexIsInAssets.IsMatch(filterPath);
			var isShader = RegexIsInShaders.IsMatch(filterPath);
			if (!isAsset && !isShader && !isExternalDependency)
			{
				Trace.TraceInformation($"Skipping '{filePath}' since it is neither in 'assets/' nor in 'shaders/' nor is it an external dependency.");
				return;
			}

			// Ensure we have no coding errors for the following cases:
			Diag.Debug.Assert(isAsset != isShader || isExternalDependency);

			// Construct the deployment and DO IT... JUST DO IT
			IFileDeployment deploy = null;
			Action<IFileDeployment> additionalInitAction = null;
			if (isShader)
			{
				if (config.TargetApi == BuildTargetApi.Vulkan)
				{
					// It's a shader and we're building for Vulkan => Special case #1
					deploy = new VkShaderDeployment();
				}
				else
				{
					Diag.Debug.Assert(config.TargetApi == BuildTargetApi.OpenGL);
					// It's a shader and we're building for OpenGL => Special case #2
					deploy = new GlShaderDeployment();
				}
			}
			else // is an asset
			{
				// Is it a model?
				try
				{
					using (AssimpContext importer = new AssimpContext())
					{
						var model = importer.ImportFile(inputFile.FullName);
						var allTextures = new HashSet<string>();
						foreach (var m in model.Materials)
						{
							if (m.HasTextureAmbient)		allTextures.Add(m.TextureAmbient.FilePath);
							if (m.HasTextureDiffuse)		allTextures.Add(m.TextureDiffuse.FilePath);
							if (m.HasTextureDisplacement)	allTextures.Add(m.TextureDisplacement.FilePath);
							if (m.HasTextureEmissive)		allTextures.Add(m.TextureEmissive.FilePath);
							if (m.HasTextureHeight)			allTextures.Add(m.TextureHeight.FilePath);
							if (m.HasTextureLightMap)		allTextures.Add(m.TextureLightMap.FilePath);
							if (m.HasTextureNormal)			allTextures.Add(m.TextureNormal.FilePath);
							if (m.HasTextureOpacity)		allTextures.Add(m.TextureOpacity.FilePath);
							if (m.HasTextureReflection)		allTextures.Add(m.TextureReflection.FilePath);
							if (m.HasTextureSpecular)		allTextures.Add(m.TextureSpecular.FilePath);
						}

						if (inputFile.FullName.Trim().EndsWith(".obj", true, System.Globalization.CultureInfo.InvariantCulture))
						{
							deploy = new ObjModelDeployment();
						}
						else
						{
							deploy = new ModelDeployment();
						}

						additionalInitAction = (theDeployment) =>
						{
							((ModelDeployment)theDeployment).SetTextures(allTextures);
						};
					}
				}
				catch (AssimpException aex)
				{
                    Trace.TraceWarning(aex.Message);
					// Maybe it is no model?!
				}

				if (null == deploy)
				{
					deploy = new CopyFileDeployment();
				}
			}

			deploy.SetInputParameters(
				config, 
				filterPath, 
				inputFile, 
				Path.Combine(config.OutputPath, filterPath, inputFile.Name));
			additionalInitAction?.Invoke(deploy);

			// We're done here => return the deployment-instance to the caller
			outDeployment = deploy;
		}

		public static bool ContainsMessagesOfType(this IList<Message> list, MessageType type)
		{
			return (from x in list where x.MessageType == type select x).Any();
		}

		public static int NumberOfMessagesOfType(this IList<Message> list, MessageType type)
		{
			return (from x in list where x.MessageType == type select x).Count();
		}

		public static bool ContainsMessagesOfType(this IList<MessageVM> list, MessageType type)
		{
			return (from x in list where x.MessageType == type select x).Any();
		}

		public static int NumberOfMessagesOfType(this IList<MessageVM> list, MessageType type)
		{
			return (from x in list where x.MessageType == type select x).Count();
		}

		public static string BuildPlatformToPartOfPath(this BuildPlatform buildPlatform)
		{
			switch (buildPlatform)
			{
				case BuildPlatform.x64:
					return "x64";
				default:
					return "???";
			}
		}

		public static void OpenFileWithSystemViewer(string path)
		{
			var info = new FileInfo(path);
			if (!info.Exists)
			{
				return;
			}

			Process.Start(info.FullName);
		}

		public static void ShowDirectoryInExplorer(string path)
		{
			try
			{
				string dirPath;
				string filePath = null;
				var attr = File.GetAttributes(path);
				if (attr.HasFlag(FileAttributes.Directory))
				{
					dirPath = path;
					if (!Directory.Exists(dirPath))
					{
						return;
					}
				}
				else
				{
					var info = new FileInfo(path);
					if (!info.Directory.Exists)
					{
						return;
					}
					dirPath = info.Directory.FullName;
					filePath = info.FullName;
				}

				if (null != filePath)
				{
					string argument = "/select, \"" + filePath + "\"";
					Process.Start("explorer.exe", argument);
				}
				else
				{
					string argument = "\"" + dirPath + "\"";
					Process.Start("explorer.exe", argument);
				}
			}
			catch (Exception ex)
			{
                Trace.TraceError(ex.Message);
			}
		}
	}
}

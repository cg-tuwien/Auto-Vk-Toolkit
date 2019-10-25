using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CgbPostBuildHelper.Model
{
	/// <summary>
	/// Against which rendering API is it built?
	/// </summary>
	enum BuildTargetApi
	{
		Vulkan,
		OpenGL
	}

	/// <summary>
	/// Which build-configuration is used?
	/// </summary>
	enum BuildConfiguration
	{
		Debug,
		Release,
		Publish
	}

	/// <summary>
	/// Which platform is it being built for?
	/// </summary>
	enum BuildPlatform
	{
		x64
	}

	/// <summary>
	/// Parameters passed from the invoker
	/// </summary>
	class InvocationParams
	{
		public string CgbFrameworkPath { get; set; }
		public List<string> CgbExternalPaths { get; set; }
		public BuildTargetApi TargetApi { get; set; }
		public BuildConfiguration Configuration { get; set; }
		public BuildPlatform Platform { get; set; }
		public string VcxprojPath { get; set; }
		public string FiltersPath { get; set; }
		public string OutputPath { get; set; }
		public string ExecutablePath { get; set; }
	}
}

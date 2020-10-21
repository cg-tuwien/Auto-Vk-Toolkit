using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CgbPostBuildHelper.Model
{
	/// <summary>
	/// In order to identify some file types which need special treatment.
	/// If a file does not need special treatment, it's just set to 'Generic'.
	/// </summary>
	enum FileType
	{
		/// <summary>
		/// Just a generic asset of any type, except for the following...
		/// </summary>
		Generic,

		/// <summary>
		/// A 3D model file. It will be loaded in order to get its 
		/// linked texture files.
		/// </summary>
		Generic3dModel,

		/// <summary>
		/// A materials file, which can only exist under a parent Obj3dModel
		/// type file.
		/// </summary>
		ObjMaterials,

		/// <summary>
		/// A .fscene file, describing an ORCA scene, in the FALCOR-format
		/// </summary>
		OrcaScene,

		/// <summary>
		/// A shader file which is intended to be used with OpenGL
		/// </summary>
		GlslShaderForGl,

		/// <summary>
		/// A shader file which is intended to be used with Vulkan
		/// </summary>
		GlslShaderForVk
	}

	/// <summary>
	/// Indicates how a file has been deployed from a filesystem-point-of-view
	/// </summary>
	enum DeploymentType
	{
		/// <summary>
		/// The file has been copied to the destination, i.e. an exact copy of
		/// the file's source exists in the destination 
		/// </summary>
		Copy,

		/// <summary>
		/// A new file has been established in the destination, but it does not
		/// (neccessarily) have the same contents as the source file because it
		/// has been morphed/transformed/altered/compiled/changed w.r.t. the original. 
		/// </summary>
		MorphedCopy,

		/// <summary>
		/// A symlink has been established in the destination, which points to
		/// the original source file.
		/// </summary>
		Symlink,

		/// <summary>
		/// A dependency to another deployment.
		/// Used for #include files that are included from shader files.
		/// </summary>
		Dependency,
	}

	class FileDeploymentData
	{
		/// <summary>
		/// The path to the original file
		/// </summary>
		public string InputFilePath { get; set; }

		/// <summary>
		/// The file hash of the original file.
		/// This is stored in order to detect file changes.
		/// </summary>
		public byte[] InputFileHash { get; set; }

		/// <summary>
		/// The path where the input file is copied to
		/// </summary>
		public string OutputFilePath { get; set; }

		/// <summary>
		/// Path of the filter as it is stored in the .vcxproj.filters file
		/// </summary>
		public string FilterPath { get; set; }

		/// <summary>
		/// Which kind of file are we dealing with?
		/// </summary>
		public FileType FileType { get; set; }


		/// <summary>
		/// HOW has the file been deployed?
		/// </summary>
		public DeploymentType DeploymentType { get; set; }

		/// <summary>
		/// (Optional) Reference to a parent AssetFile entry.
		/// This will be set for automatically determined assets, 
		/// like it will happen for the textures of 3D models.
		/// 
		/// Whenever the parent asset changes, also its child assets
		/// have to be updated. This is done via those references.
		/// </summary>
		public FileDeploymentData Parent { get; set; }

		/// <summary>
		/// Messages which occured during file deployment or during file update
		/// </summary>
		public List<Message> Messages { get; } = new List<Message>();
	}
}

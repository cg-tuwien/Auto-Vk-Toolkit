using CgbPostBuildHelper.ViewModel;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using CgbPostBuildHelper.Utils;
using System.Windows.Media;
using System.Windows.Input;
using System.Windows;
using CgbPostBuildHelper.View;
using System.Diagnostics;
using CgbPostBuildHelper.Model;

namespace CgbPostBuildHelper.ViewModel
{
	/// <summary>
	/// Data about exactly one specific asset file.
	/// </summary>
	class FileDeploymentDataVM : FileVM
	{
		Model.FileDeploymentData _model;

		public FileDeploymentDataVM(Model.FileDeploymentData model, CgbAppInstanceVM inst)
		{
			_model = model;
			foreach (var msg in _model.Messages)
			{
				Messages.Add(new MessageVM(inst, msg));
			}
		}

		/// <summary>
		/// The path to the original file
		/// </summary>
		public string InputFilePath
		{
			get => _model.InputFilePath;
			set => SetProperty(_model, m => m.InputFilePath, value);
		}

		/// <summary>
		/// The file hash of the original file.
		/// This is stored in order to detect file changes.
		/// </summary>
		public byte[] InputFileHash
		{
			get => _model.InputFileHash;
			set => SetProperty(_model, m => m.InputFileHash, value);
		}

		/// <summary>
		/// The path where the input file is copied to
		/// </summary>
		public string OutputFilePath
		{
			get => _model.OutputFilePath;
			set => SetProperty(_model, m => m.OutputFilePath, value);
		}

		/// <summary>
		/// Path of the filter as it is stored in the .vcxproj.filters file
		/// </summary>
		public string FilterPath
		{
			get => _model.FilterPath;
			set => SetProperty(_model, m => m.FilterPath, value);
		}

		public string FilterPathPlusFileName => Path.Combine(FilterPath ?? "?", new FileInfo(InputFilePath).Name);

		/// <summary>
		/// Which kind of file are we dealing with?
		/// </summary>
		public FileType FileType
		{
			get => _model.FileType;
			set => SetProperty(_model, m => m.FileType, value);
		}

		public string FileTypeDescription
		{
			get
			{
				switch (this.FileType)
				{
					case FileType.Generic:
						return "Generic file";
					case FileType.Generic3dModel:
						return "3D model file";
					case FileType.ObjMaterials:
						return "Materials file (.mat) of an .obj 3d model";
					case FileType.OrcaScene:
						return "ORCA Scene file";
					case FileType.GlslShaderForGl:
						return "GLSL shader for OpenGL (potentially modified from Vk-GLSL original)";
					case FileType.GlslShaderForVk:
						return "GLSL shader for Vulkan, compiled to SPIR-V";
				}
				return "?FileType?";
			}
		}

		/// <summary>
		/// HOW has the file been deployed?
		/// </summary>
		public DeploymentType DeploymentType
		{
			get => _model.DeploymentType;
			set => SetProperty(_model, m => m.DeploymentType, value);
		}

		public string DeploymentTypeDescription
		{
			get
			{
				switch (this.DeploymentType)
				{
					case DeploymentType.Symlink:
						return "Symlink pointing to original";
					case DeploymentType.MorphedCopy:
						return "New file (compiled or otherwise modified w.r.t. original)";
					case DeploymentType.Copy:
						return "Copy of the original";
					case DeploymentType.Dependency:
						return "File-Dependency";
				}
				return "?DeploymentType?";
			}
		}

		/// <summary>
		/// (Optional) Reference to a parent AssetFile entry.
		/// This will be set for automatically determined assets, 
		/// like it will happen for the textures of 3D models.
		/// 
		/// Whenever the parent asset changes, also its child assets
		/// have to be updated. This is done via those references.
		/// </summary>
		public FileDeploymentData Parent
		{
			get => _model.Parent;
			set => SetProperty(_model, m => m.Parent, value);
		}

		/// <summary>
		/// Messages which occured during file deployment or during file update
		/// </summary>
		public ObservableCollection<MessageVM> Messages { get; } = new ObservableCollection<MessageVM>();

		public string MessagesErrorInfo
		{
			get
			{
				var n = Messages.NumberOfMessagesOfType(MessageType.Error);
				return $"{n} error{(n == 1 ? "" : "s")}";
			}
		}
		public string MessagesInformationInfo
		{
			get
			{
				var n = Messages.NumberOfMessagesOfType(MessageType.Information);
				return $"{n} info{(n == 1 ? "" : "s")}";
			}
		}
		public string MessagesSuccessInfo
		{
			get
			{
				var n = Messages.NumberOfMessagesOfType(MessageType.Success);
				return $"{n} success-message{(n == 1 ? "" : "s")}";
			}
		}
		public string MessagesWarningInfo
		{
			get
			{
				var n = Messages.NumberOfMessagesOfType(MessageType.Warning);
				return $"{n} warning{(n == 1 ? "" : "s")}";
			}
		}

		public Brush MessagesErrorInfoColor => Messages.NumberOfMessagesOfType(MessageType.Error) > 0 ? View.Constants.ErrorBrushDark : Brushes.LightGray;
		public Brush MessagesInformationInfoColor => Messages.NumberOfMessagesOfType(MessageType.Information) > 0 ? View.Constants.InfoBrushDark : Brushes.LightGray;
		public Brush MessagesSuccessInfoColor => Messages.NumberOfMessagesOfType(MessageType.Success) > 0 ? View.Constants.SuccessBrushDark : Brushes.LightGray;
		public Brush MessagesWarningInfoColor => Messages.NumberOfMessagesOfType(MessageType.Warning) > 0 ? View.Constants.WarningBrushDark : Brushes.LightGray;

		public ICommand ShowFileMessagesCommand
		{
			get => new DelegateCommand(_ => 
			{
				var window = new View.WindowToTheTop
				{
					Width = 480, Height = 320,
					Title = "Messages for file " + FilterPathPlusFileName
				};
				window.InnerContent.Content = new MessagesList()
				{
					DataContext = new 
					{ 
						Items = Messages,
						DismissCommand = new Func<ICommand>(() => null)() // Evaluate to null in order to set the >>>DISMISS>>> buttons to collapsed
					}
				};
				window.Show();
			});
		}
	}

}

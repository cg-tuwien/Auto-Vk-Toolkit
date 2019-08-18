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
	interface IFileDeployment
	{
		void SetInputParameters(InvocationParams config, string filterPath, FileInfo inputFile, string outputFilePath);
		void Deploy();
		List<FileDeploymentData> FilesDeployed { get; }
		string DesignatedOutputPath { get; }
	}

}

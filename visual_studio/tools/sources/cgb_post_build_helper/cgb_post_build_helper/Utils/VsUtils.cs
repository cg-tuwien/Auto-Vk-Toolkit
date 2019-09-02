using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using EnvDTE;
using EnvDTE80;
using System.Diagnostics;

namespace CgbPostBuildHelper.Utils
{
	static class VsUtils
	{
		private static readonly bool SelectLine = true;

		[DllImport("ole32.dll")]
		private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable prot);

		[DllImport("ole32.dll")]
		private static extern int CreateBindCtx(int reserved, out IBindCtx ppbc);

		private static bool AreFileNamesEqual(string filepath1, string filepath2)
		{
			return CgbUtils.NormalizePath(filepath1) == CgbUtils.NormalizePath(filepath2);
		}

		// Credits: https://stackoverflow.com/questions/1626458/how-to-attach-a-debugger-dynamically-to-a-specific-process
		private static List<EnvDTE80.DTE2> GetCurrentlyRunningVisualStudioInstances()
		{
			List<EnvDTE80.DTE2> vsInstances = new List<EnvDTE80.DTE2>();

			IntPtr numFetched = IntPtr.Zero;
			IRunningObjectTable runningObjectTable;
			IEnumMoniker monikerEnumerator;
			IMoniker[] monikers = new IMoniker[1];

			GetRunningObjectTable(0, out runningObjectTable);
			runningObjectTable.EnumRunning(out monikerEnumerator);
			monikerEnumerator.Reset();

			while (monikerEnumerator.Next(1, monikers, numFetched) == 0)
			{
				IBindCtx ctx;
				CreateBindCtx(0, out ctx);

				string runningObjectName;
				monikers[0].GetDisplayName(ctx, null, out runningObjectName);

				object runningObjectVal;
				runningObjectTable.GetObject(monikers[0], out runningObjectVal);

				if (runningObjectVal is EnvDTE80.DTE2 && runningObjectName.StartsWith("!VisualStudio"))
				{
					int currentProcessId = int.Parse(runningObjectName.Split(':')[1]);

					var instance = (EnvDTE80.DTE2)runningObjectVal;
					vsInstances.Add(instance);
				}
			}

			return vsInstances;
		}

		// Credits: http://www.wwwlicious.com/2011/03/29/envdte-getting-all-projects-html/
		private static IList<Project> GetAllProjectsOfVisualStudioInstance(EnvDTE80.DTE2 vsInstance)
		{
			Projects projects = vsInstance.Solution.Projects;
			List<Project> list = new List<Project>();
			var item = projects.GetEnumerator();
			while (item.MoveNext())
			{
				var project = item.Current as Project;
				if (project == null)
				{
					continue;
				}

				if (project.Kind == ProjectKinds.vsProjectKindSolutionFolder)
				{
					list.AddRange(GetSolutionFolderProjects(project));
				}
				else
				{
					list.Add(project);
				}
			}

			return list;
		}

		// Credits: http://www.wwwlicious.com/2011/03/29/envdte-getting-all-projects-html/
		private static IEnumerable<Project> GetSolutionFolderProjects(Project solutionFolder)
		{
			List<Project> list = new List<Project>();
			for (var i = 1; i <= solutionFolder.ProjectItems.Count; i++)
			{
				var subProject = solutionFolder.ProjectItems.Item(i).SubProject;
				if (subProject == null)
				{
					continue;
				}

				// If this is another solution folder, do a recursive call, otherwise add
				if (subProject.Kind == ProjectKinds.vsProjectKindSolutionFolder)
				{
					list.AddRange(GetSolutionFolderProjects(subProject));
				}
				else
				{
					list.Add(subProject);
				}
			}
			return list;
		}

		/// <summary>
		/// Iterates through all the currently active Visual Studio instances.
		/// If an instance shows the desired file, it will be brought to the front and maybe a line highlighted.
		/// </summary>
		/// <param name="fileToActivate">The file to be shown</param>
		/// <param name="lineToHighlight">Optional parameter: If set, highlight that line!</param>
		/// <returns>True if the file was successfully activated in a Visual Studio instance</returns>
		public static bool ActivateFileInRunningVisualStudioInstances(string fileToActivate, int? lineToHighlight)
		{
			var vsInstances = GetCurrentlyRunningVisualStudioInstances();
			bool selected = false;
			foreach (EnvDTE80.DTE2 vsInst in vsInstances)
			{
				foreach (Document doc in vsInst.Documents)
				{
					if (AreFileNamesEqual(doc.FullName, fileToActivate))
					{
						doc.ActiveWindow.Visible = true;
						doc.Activate();
						if (lineToHighlight.HasValue)
						{
							((EnvDTE.TextSelection)vsInst.ActiveDocument.Selection).GotoLine(lineToHighlight.Value, SelectLine);
						}
						selected = true;
					}
				}
			}
			return selected;
		}

		/// <summary>
		/// Interates throught the currently active Visual Studio instances, looking for the one
		/// which has the project with the given path opened. If found, the desired file will be 
		/// opened in that Visual Studio instance (and a line will be highlighted, if requested).
		/// </summary>
		/// <param name="specificProjectPath">The project which we are looking for</param>
		/// <param name="fileToActivate">The file we want to show</param>
		/// <param name="lineToHighlight">The line inside the file to highlight</param>
		/// <returns>True if the file was opened in a Visual Studio instance, false if no suitable Visual Studio instance has been found.</returns>
		public static bool OpenFileInSpecificVisualStudioInstance(string specificProjectPath, string fileToActivate, int? lineToHighlight)
		{
			var vsInstances = GetCurrentlyRunningVisualStudioInstances();
			foreach (EnvDTE80.DTE2 vsInst in vsInstances)
			{
				var vsProjects = GetAllProjectsOfVisualStudioInstance(vsInst);
				foreach (Project vsProj in vsProjects)
				{
					if (string.IsNullOrWhiteSpace(vsProj.FileName))
					{
						continue;
					}
					if (AreFileNamesEqual(vsProj.FileName, specificProjectPath))
					{
						vsInst.ItemOperations.OpenFile(fileToActivate);
						if (lineToHighlight.HasValue)
						{
							((EnvDTE.TextSelection)vsInst.ActiveDocument.Selection).GotoLine(lineToHighlight.Value, SelectLine);
						}
						return true;
					}
				}
			}
			return false;
		}

		/// <summary>
		/// Opens up a new instance of Visual Studio, opens the desired file in that instance,
		/// and selects the line (if requested).
		/// </summary>
		/// <param name="fileToActivate">File to open</param>
		/// <param name="lineToHighlight">Line inside the file to highlight</param>
		/// <returns></returns>
		public static bool OpenFileInExistingVisualStudioInstance(string fileToActivate, int? lineToHighlight)
		{
			try
			{
				EnvDTE80.DTE2 vsInst = (EnvDTE80.DTE2)System.Runtime.InteropServices.Marshal.GetActiveObject("VisualStudio.DTE");
				if (null == vsInst?.MainWindow)
				{
					return false;
				}
				vsInst.MainWindow.Activate();
				EnvDTE.Window w = vsInst.ItemOperations.OpenFile(fileToActivate, EnvDTE.Constants.vsViewKindTextView);
				if (null == w || null == vsInst.ActiveDocument)
				{
					return false;
				}
				if (lineToHighlight.HasValue)
				{
					((EnvDTE.TextSelection)vsInst.ActiveDocument.Selection).GotoLine(lineToHighlight.Value, SelectLine);
				}
				return true;
			}
			catch (Exception e)
			{
                Trace.TraceError(e.Message);
				return false;
			}
		}
	}
}

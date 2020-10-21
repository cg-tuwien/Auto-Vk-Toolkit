using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CgbPostBuildHelper.Model
{
	enum MessageType
	{
		Success,
		Information,
		Warning,
		Error,
	}

	class Message
	{
		public static Message Create(MessageType typeOfMessage, string text, Action additionalInfoCommand)
		{
			return new Message()
			{
				MessageType = typeOfMessage,
				MessageText = text,
				Action = additionalInfoCommand
			};
		}

		public static Message Create(MessageType typeOfMessage, string text, Action additionalInfoCommand, string filename, bool canBeEditedInVisualStudio = false, int? lineNumberInFile = null)
		{
			var m = Create(typeOfMessage, text, additionalInfoCommand);
			m.FilenameForFileActions = filename;
			m.FileCanBeEditedInVisualStudio = canBeEditedInVisualStudio;
			m.LineNumberInFile = lineNumberInFile;
			return m;
		}

		private readonly DateTime _createDate = DateTime.Now;

		public DateTime CreateDate => _createDate;

		public string MessageText { get; set; }

		public MessageType MessageType { get; set; }

		public Action Action { get; set; }

		public string FilenameForFileActions { get; set; }

		public bool FileCanBeEditedInVisualStudio { get; set; }

		public int? LineNumberInFile { get; set; }
	}
}

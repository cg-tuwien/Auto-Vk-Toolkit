using CreateNewProject.Utils;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;

namespace CreateNewProject.ViewModel
{
	class MessageViewModel : BindableBase
	{
		public static MessageViewModel CreateSuccess(string text)
		{
			return new MessageViewModel
			{
				_messageColor = Brushes.DarkSeaGreen,
				_messageText = text
			};
		}

		public static MessageViewModel CreateInfo(string text)
		{
			return new MessageViewModel
			{
				_messageColor = Brushes.DimGray,
				_messageText = text
			};
		}

		public static MessageViewModel CreateWarning(string text)
		{
			return new MessageViewModel
			{
				_messageColor = Brushes.Orange,
				_messageText = text
			};
		}

		public static MessageViewModel CreateError(string text)
		{
			return new MessageViewModel
			{
				_messageColor = Brushes.Red,
				_messageText = text
			};
		}

		public Brush MessageColor
		{
			get => _messageColor;
			set => SetProperty(ref _messageColor, value);
		}

		public string MessageText
		{
			get => _messageText;
			set => SetProperty(ref _messageText, value);
		}

		private Brush _messageColor;
		private string _messageText;
	}
}

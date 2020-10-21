using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Linq.Expressions;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace CgbPostBuildHelper.Utils
{
	/// <summary>
	///     Implementation of <see cref="INotifyPropertyChanged" /> to simplify models.
	/// </summary>
	public abstract class BindableBase : INotifyPropertyChanged
	{
		private List<ICommand> _allCommands = null;
		protected IList<ICommand> AllCommands => _allCommands;

		/// <summary>
		/// Gathers all properties of type ICommand into the AllCommands list
		/// </summary>
		protected void GatherAllCommands()
		{
			// We need all commands in a list in order to manually update their CanExecute states if something happens (e.g. DB connection lost)
			// See method FireAllCanExecuteChanged()
			if (null == _allCommands)
			{
				_allCommands = new List<ICommand>();
			}
			else
			{
				_allCommands.Clear();
			}
			foreach (var prop in this.GetType().GetProperties().Where(p => typeof(ICommand).IsAssignableFrom(p.PropertyType)))
			{
				ICommand propInst = prop.GetValue(this) as ICommand;
				if (null != propInst)
				{
					_allCommands.Add(propInst);
				}
			}
		}

		/// <summary>
		/// For each (previously gathered) command of type DelegateCommand or Universal Command, 
		/// raises the CanExecuteChanged event
		/// </summary>
		public virtual void FireAllCanExecuteChanged()
		{
			if (null == _allCommands)
			{
				GatherAllCommands();
			}
			if (null == _allCommands)
			{
				return;
			}
			foreach (var cmd in _allCommands)
			{
				var raiseable = cmd as IRaiseCanExecuteChanged;
				raiseable?.RaiseCanExecuteChanged();
			}
		}

		/// <summary>
		///     Multicast event for property change notifications.
		/// </summary>
		public event PropertyChangedEventHandler PropertyChanged;

		/// <summary>
		///     Checks if a property already matches a desired value.  Sets the property and
		///     notifies listeners only when necessary.
		/// </summary>
		/// <typeparam name="T">Type of the property.</typeparam>
		/// <param name="storage">Reference to a property with both getter and setter.</param>
		/// <param name="value">Desired value for the property.</param>
		/// <param name="propertyName">
		///     Name of the property used to notify listeners.  This
		///     value is optional and can be provided automatically when invoked from compilers that
		///     support CallerMemberName.
		/// </param>
		/// <returns>
		///     True if the value was changed, false if the existing value matched the
		///     desired value.
		/// </returns>
		protected bool SetProperty<T>(ref T storage, T value, [CallerMemberName] string propertyName = null)
		{
			if (Equals(storage, value))
			{
				return false;
			}

			storage = value;
			this.OnPropertyChanged(propertyName);
			return true;
		}

		protected bool SetProperty<TModel, TProperty>(TModel model, Expression<Func<TModel, TProperty>> expr, TProperty value, [CallerMemberName] string propertyName = null)
		{
			if (null == model)
			{
				return false;
			}

			var propVal = expr.Compile().Invoke(model);
			if (Equals(propVal, value))
			{
				return false;
			}

			var p = Expression.Parameter(typeof(TProperty), "v");
			var assignExpr = Expression.Lambda<Action<TModel, TProperty>>(Expression.Assign(expr.Body, p), expr.Parameters[0], p);
			assignExpr.Compile().Invoke(model, value);
			this.OnPropertyChanged(propertyName);
			return true;
		}

		protected void IssueOnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			this.OnPropertyChanged(propertyName);
		}

		public void IssueAllOnPropertyChanged()
		{
			this.OnPropertyChanged(null);
		}

		/// <summary>
		///     Notifies listeners that a property value has changed.
		/// </summary>
		/// <param name="propertyName">
		///     Name of the property used to notify listeners.  This
		///     value is optional and can be provided automatically when invoked from compilers
		///     that support <see cref="CallerMemberNameAttribute" />.
		/// </param>
		protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			PropertyChangedEventHandler eventHandler = this.PropertyChanged;
			if (eventHandler != null)
			{
				eventHandler(this, new PropertyChangedEventArgs(propertyName));
			}
		}

		public void RemoveAllSubscribersToPropertyChanged()
		{
			this.PropertyChanged = null;
		}
	}
}

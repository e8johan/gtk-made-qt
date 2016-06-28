#include <QXmlStreamReader>
#include <QUiLoader>

// Class to handle XML input and turn it into a tree of widgets.
class GQTUIParser : public QXmlStreamReader
{
	QWidget *tree_parent;
	QMap<QString, QString> widget_type_map;
	QMap<QString, QWidget *> widget_map;
 public:
	GQTUIParser() : QXmlStreamReader()
	{
		this->tree_parent = NULL;
		widget_type_map["menubar"] = "QMenuBar";
		widget_type_map["menu"] = "QMenu";
//		widget_type_map["menuitem"] = "";
//		widget_type_map["menuitem"] = "QAction"; // oh, this is going to be fun.
	}

	GtkWidget *parseUi()
	{
		QUiLoader widgetloader;
		int current_depth = 0;
		int parent_depth = 0;
		QWidget *parent = NULL;
		QWidget *current;

		while (!this->atEnd())
		{
			this->readNext();

			if (this->isStartElement())
			{
				current_depth++;
				qDebug("GQTUIParser: Got %s with name %s", qPrintable(this->name().toString()), qPrintable(this->attributes().value("name").toString()));
				QMap<QString, QString>::ConstIterator it = widget_type_map.find(this->name().toString());

				if (it == widget_type_map.end())
					continue;

				current = widgetloader.createWidget(*it, NULL, this->attributes().value("name").toString());


				// XXX: This +1 smells a bit magical
				if (current_depth > parent_depth + 1)
				{
					qDebug("GQTUIParser: reset parent to %s as %s", qPrintable(this->name().toString()), qPrintable(*it));
					parent = current;
					parent_depth = current_depth;
				}

				qDebug("Created %p, parent is %p", current, parent);

				// Set it in the map for quick lookup by name
				this->widget_map[this->attributes().value("name").toString()] = current;

				// Set the root node of the tree
				if (parent == current)
				{
					this->tree_parent = parent;

					// Don't do anything else, there is no parent node to deal with
					continue;
				}
				else
				{
					// Set the parent of this widget. We don't want to do this for the root node, for obvious reasons.
					this->doParenting(current, parent);
				}

			}

			if (this->isEndElement())
			{
				current_depth--;
			}
		}

		return NULL;
	}

	void doParenting(QWidget *current, QWidget *parent)
	{
		// Special case: we can't use QWidget::setParent on menu stuff.
		QMenuBar *menubar = dynamic_cast<QMenuBar *>(parent);
		if (menubar)
		{
			QMenu *test = dynamic_cast<QMenu *>(current);

			// if they're not adding a QMenu to a QMenuBar.. wtf
			Q_ASSERT(test && parent != current);
			test->setTitle(this->attributes().value("name").toString());

			menubar->addMenu(test);

			test->QObject::setParent(menubar);
		}
		else
		{
			qDebug("Parent is not QMenuBar");
			current->setParent(parent);
		}

	}

	GtkWidget *getWidget(const QString &name) const
	{
		QMap<QString, QWidget *>::ConstIterator it = this->widget_map.constFind(name);

		qDebug("GQTUIParser: Finding widget %s by name", qPrintable(name));

		Q_ASSERT(it != this->widget_map.end());

		qDebug("GQTUIParser: returning %p", *it);

		return *it;
	}
};

typedef GQTUIParser GtkUIManager;


GtkUIManager *gtk_ui_manager_new()
{
	return new GtkUIManager();
}

void gtk_ui_manager_insert_action_group(GtkUIManager *uiman, GtkActionGroup *actions, gint ignored)
{

}

void gtk_ui_manager_add_ui_from_string(GtkUIManager *uiman, const gchar *buffer, gssize length, GError **error)
{
	Q_ASSERT(uiman && buffer && error);

	// GTK allows -1 if it's NULL terminated
	if (length == -1)
		length = strlen(buffer);

	Q_ASSERT(length > 0);

	uiman->addData(QByteArray(buffer, length));

	uiman->parseUi();
}

GtkWidget *gtk_ui_manager_get_widget(GtkUIManager *uiman, const gchar *widget_name)
{
	Q_ASSERT(uiman && widget_name);

	// XXX: widget_name must be a URI-like transversal of the XML, we don't yet support that, hence the ++.
	// i.e. widget_name looks like /MainMenuBar
	return uiman->getWidget(++widget_name);
}

#define GtkAccelGroup void
GtkAccelGroup *gtk_ui_manager_get_accel_group(GtkUIManager *uiman)
{

}

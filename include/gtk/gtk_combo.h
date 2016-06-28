/* New combo box implementation. Apparantly GtkCombo wasn't good enough ;p */
class GQTComboBox : public QComboBox
{
	Q_OBJECT

 protected:
 public:
	GQTComboBox() : QComboBox()
	{
		this->connect(this, SIGNAL(currentIndexChanged(int)), SLOT(gqt_currentIndexChanged(int)));
	}

 public slots:
	 void gqt_currentIndexChanged(int index)
	 {
		// Fire signal
		gqt_signal_execute(this, "changed");
	 }
};

#include <gtk/moc_gtk_combo.h>

typedef GQTComboBox GtkComboBox;
#define GTK_COMBO_BOX(x) dynamic_cast<GQTComboBox *>(x)

GtkWidget *gtk_combo_box_new_text()
{
	return new GtkComboBox();
}

void gtk_combo_box_append_text(GtkComboBox *combo_box, const gchar *text)
{
	Q_ASSERT(combo_box && text);

	combo_box->addItem(text);
}

void gtk_combo_box_insert_text(GtkComboBox *combo_box, gint position, const gchar *text)
{
	Q_ASSERT(combo_box && text);

	combo_box->insertItem(position, text);
}

void gtk_combo_box_prepend_text(GtkComboBox *combo_box, const gchar *text)
{
	Q_ASSERT(combo_box && text);

	combo_box->insertItem(0, text);
}

void gtk_combo_box_remove_text(GtkComboBox *combo_box, gint position)
{
	Q_ASSERT(combo_box);

	combo_box->removeItem(position);
}

void gtk_combo_box_set_active(GtkComboBox *combo_box, gint index_)
{
	Q_ASSERT(combo_box);

	combo_box->setCurrentIndex(index_);
}

gint gtk_combo_box_get_active(GtkComboBox *combo_box)
{
	Q_ASSERT(combo_box);

	return combo_box->currentIndex();
}

/**************************************************************
 * This code implements obsolete GTK interfaces.
 *************************************************************/
typedef GQTComboBox GtkCombo;
#define GTK_COMBO(x) dynamic_cast<GQTComboBox *>(x)

GtkWidget *gtk_combo_new() MARK_DEPRECATED;
GtkWidget *gtk_combo_new()
{
	return new GtkCombo();
}

static void foreach_iter(gpointer data, gpointer user_data)
{
	GtkCombo *g = (GtkCombo *)user_data;

	g->addItem((const char *)data);
}

void gtk_combo_set_popdown_strings(GtkCombo *combo, GList *strings) MARK_DEPRECATED;
void gtk_combo_set_popdown_strings(GtkCombo *combo, GList *strings)
{
	Q_ASSERT(combo && strings);

	combo->clear();

	g_list_foreach(strings, foreach_iter, combo);
}

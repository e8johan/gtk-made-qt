typedef QActionGroup GtkActionGroup;
//typedef QAction GtkActionEntry;

// This really needs to be a struct thanks to the way userland code (and things like gtk_action_group_add_actions() works).
typedef struct
{
	const gchar     *name;
	const gchar     *stock_id;
	const gchar     *label;
	const gchar     *accelerator;
	const gchar     *tooltip;
	GCallback  callback;
} GtkActionEntry;


GtkActionGroup *gtk_action_group_new(const gchar *name)
{
	// XXX 2: we ignore name, this requires implementing, perhaps via QObject::setProperty()?
	return NULL;
}

void gtk_action_group_add_actions(GtkActionGroup *ag, GtkActionEntry *actions, gint action_count, gpointer user_data)
{

}

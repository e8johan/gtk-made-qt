
static QApplication *gdk_app = NULL;

void gtk_init(int *argc, char ***argv)
{
	// Don't allow double calls
	Q_ASSERT(!gdk_app);

	gdk_app = new QApplication(*argc, *argv);
}

void gtk_main()
{
	// XXX: this is quite probably wrong.
	gdk_app->exec();
	delete gdk_app;
}

void gtk_main_quit()
{
	// This will break out of gdk_app->exec().
	QApplication::exit(0);
}

gboolean gtk_true()
{
	return TRUE;
}

gboolean gtk_false()
{
	return FALSE;
}



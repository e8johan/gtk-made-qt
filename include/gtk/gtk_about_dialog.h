/* 
 * gqt, implementing the GTK APIs through Qt
 * Copyright (C) Robin Burchell
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Lesser GNU General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

class GQTAboutDialog;
typedef GQTAboutDialog GtkAboutDialog;


class GQTAboutDialog : public GQTDialog
{
 private:
	QLabel *program_name;
	QLabel *version;
	QLabel *comments;
	QTextEdit *licence;

	QVBoxLayout *layout;
 public:
	GQTAboutDialog() : GQTDialog()
	{
		QHBoxLayout *h;

		this->setWindowTitle("About");
		this->layout = new QVBoxLayout(this);

		// FIXME: there is some error about 'Attempting to add QLayout "" to QWidget "", which already has a layout.
		// Not sure what this is, doesn't seem to break anything, needs solving anyway.
		h = new QHBoxLayout(this);
		// FIXME: to match GTK, these two widgets need their text bolded.
		h->addWidget((this->program_name = new QLabel(this)), 0, Qt::AlignCenter);
		h->addWidget((this->version = new QLabel(this)), 0, Qt::AlignCenter);

		this->layout->addLayout(h);
		this->layout->addWidget((this->comments = new QLabel(this)), 0, Qt::AlignCenter);

		/*
		 * FIXME: to mimic GTK, we should create a second QHBox, add a 'licence' and 'close' button
		 * and display licence in a QTextView in a new window if clicked.
		 *
		 * I won't do that now because I don't want to MOC a second file atm.
		 */
		this->layout->addWidget((this->licence = new QTextEdit(this)));

		this->setLayout(this->layout);
	}

	~GQTAboutDialog()
	{
	}

	void setProgramName(QString name)
	{
		this->program_name->setText(name);
	}

	void setLicence(QString licence)
	{
		this->licence->setText(licence);
	}

	void setVersion(QString version)
	{
		this->version->setText(version);
	}

	void setComments(QString comments)
	{
		this->comments->setText(comments);
	}
};

GtkWidget *gtk_about_dialog_new()
{
	return new GQTAboutDialog();
}

void gtk_about_dialog_set_program_name(GtkAboutDialog *diag, const gchar *s)
{
	Q_ASSERT(diag && s);

	diag->setProgramName(s);
}

void gtk_about_dialog_set_license(GtkAboutDialog *diag, const gchar *s)
{
	Q_ASSERT(diag && s);

	diag->setLicence(s);
}

void gtk_about_dialog_set_version(GtkAboutDialog *diag, const gchar *s)
{
	Q_ASSERT(diag && s);

	diag->setVersion(s);
}

void gtk_about_dialog_set_comments(GtkAboutDialog *diag, const gchar *s)
{
	Q_ASSERT(diag && s);

	diag->setComments(s);
}

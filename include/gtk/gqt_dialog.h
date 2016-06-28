
class GQTDialog : public QWidget
{
 protected:
	void closeEvent(QCloseEvent *event)
	{
		// Fire response signal.
		gqt_signal_execute(this, "response");
	}
};

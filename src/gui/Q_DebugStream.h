#ifndef Q_DEBUGSTREAM_HPP
#define Q_DEBUGSTREAM_HPP
#include <iostream>
#include <streambuf>
#include <string>
#include <mutex>
#include <QTextEdit>
#include <QObject>

class QDebugStream : public QObject, public std::basic_streambuf<char>
{
	Q_OBJECT
public:
	QDebugStream(std::ostream &stream) : m_stream(stream)
	{
		m_old_buf = stream.rdbuf();
		stream.rdbuf(this);
	}

	~QDebugStream()
	{
		m_stream.rdbuf(m_old_buf);
	}

	/*static void registerQDebugMessageHandler(){
		qInstallMessageHandler(myQDebugMessageHandler);
	}*/
Q_SIGNALS:
	void errorPrinted(QString err);

private:
	/*static void myQDebugMessageHandler(QtMsgType, const QMessageLogContext
    &, const QString &msg)
    {
	std::cout << msg.toUtf8().toStdString();
    }*/

protected:
	// This is called when a std::endl has been inserted into the stream
	virtual int_type overflow(int_type v)
	{
		if (v == '\n') {
			Q_EMIT errorPrinted(QString('\n'));
		}
		return v;
	}


	virtual std::streamsize xsputn(const char *p, std::streamsize n)
	{
		QString str(p);
		Q_EMIT errorPrinted(str.trimmed());
		return n;
	}

private:
	std::ostream &m_stream;
	std::streambuf *m_old_buf;
};

#endif // Q_DEBUGSTREAM_HPP

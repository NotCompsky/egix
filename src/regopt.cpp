#include "regopt.hpp"
#include <QProcess>
#include <QMessageBox>


void optimise_regex(QString& data,  QString& result){
	QProcess regtrie;
	QStringList args;
	for (int i = data.length();  i != 0;  ){
		--i;
		if (data[i] == QChar('\n')){
			data[i] = QChar('|');
		}
	}
	args << data;
	regtrie.start("regopt.pl", args);
	if (!regtrie.waitForFinished()){
		QMessageBox::warning(0,  "Error",  "Cannot execute regopt.pl");
		return;
	}
	result = regtrie.readAllStandardOutput();

	/* Code to replace the groups that match start of string '^' with groups that do not */
	int i = 0;
	while(result[i] == QChar('(')  &&  result[i+1] == QChar('?')  &&  result[i+2] == QChar('^')  && result[i+3] == QChar(':')){
		i += 4;
	}
	int j = i / 4;
	for (auto k = j;  k != 0;  --k){
		result[--i] = QChar(':');
		result[--i] = QChar('?');
		result[--i] = QChar('(');
	}
	result.remove(0, j); // Remove (in-place) the now-unused characters

	regtrie.close();
}

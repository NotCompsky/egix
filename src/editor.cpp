/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */


#include "egix/editor.hpp"
#include "highlighter.hpp"
#include "sql_name_dialog.hpp"
#include "regopt.hpp"
#include "msgbox.hpp"
#include "3rdparty/codeeditor.hpp"

#include <compsky/mysql/query.hpp>
#include <compsky/regex/named_groups.hpp>

#include <boost/regex.hpp>

#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringRef>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextStream>



namespace filter_comment_body {
	extern boost::basic_regex<char, boost::cpp_regex_traits<char>>* regexpr;
}

static const QString help_text = 
	"Supports boost::regex Perl syntax, with Python named groups (?P<name>).\n"
	"The group syntax is more flexible than Python's - you can use whatever characters you please, save for '&' and '<', and can use the same group name for multiple groups.\n"
	"Group names are indicated with bold blue text.\n"
	"\n"
	"The first spaces and tabs of each line are ignored, except if the newline was escaped.\n"
	"This is indicated by green highlighting (NOTE: a bug currently means that space after escaped newlines are also (wrongly) highlighted, however this only affects the syntax highlighter and not the pre-processor itself).\n"
	"\n"
	"All text after a (space or tab) followed by '#' is ignored, and all unescaped preceding spaces and tabs too.\n"
	"This is indicated by green highlighting.\n"
	"\n"
	"Unescaped trailing whitespace is NOT ignored; but since it may be accidental, is highlighted cyan.\n"
	"\n"
	"Only \\\\, \\n, \\r, \\t and \\v are recognised escapes sequences. \\{, \\}, \\( and \\) are simply parsed as their latter character.\n"
	"They are indicated in red.\n"
	"\n"
	"Variable declarations have an almost identical syntax to named groups: {?P<varname>actual string that will be copied}\n"
	"These encompass strings which can then be copy-pasted using an unescaped ${VARNAME}, substituting VARNAME for the exact name of the variable. This will copy everything (aside from the variable name) within the curly braces - for instance, {?P<foobar>hello}${foobar} would result in the string 'hellohello' appearing in the final regex.\n"
	"Such variables can also be declared seperately to the regex file in the 'Vars' menu.\n"
	"Variable declarations must not share names with each other.\n"
;


void RegexEditor::ensure_buf_sized(const size_t buf_sz){
	if (buf_sz < this->buf_sz)
		return;
	// Ensure there is sufficient space to run the compsky::mysql::exec commands below
	this->buf_sz = 2*buf_sz;
	void* dummy = malloc(this->buf_sz);
	if (unlikely(dummy == nullptr))
		exit(4096);
	free(this->buf);
	this->buf = (char*)dummy;
}


RegexEditor::RegexEditor(QWidget* parent) : QDialog(parent) {
	this->buf = (char*)malloc(4096);
	if(unlikely(this->buf == nullptr))
		exit(4096);
	this->itr = buf;
	
	QVBoxLayout* l = new QVBoxLayout;
	
	this->text_editor = new CodeEditor(this);

	QFont font;
	font.setStyleHint(QFont::Monospace);
	font.setFixedPitch(true);
	this->text_editor->setFont(font);
	QFontMetrics metrics(font);
	this->text_editor->setTabStopWidth(metrics.width("    "));

	l->addWidget(this->text_editor);
	new RegexEditorHighlighter(this->text_editor->document());

	{
	QHBoxLayout* hbox = new QHBoxLayout;

	{
	QPushButton* btn = new QPushButton("Help", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::display_help);
	hbox->addWidget(btn);
	}

	{
	QPushButton* btn = new QPushButton("Find", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::find_text);
	hbox->addWidget(btn);
	}

	this->want_optimisations = new QCheckBox("Optimise", this);
	l->addWidget(this->want_optimisations);

	{
	QPushButton* btn = new QPushButton("Test", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::test_regex);
	hbox->addWidget(btn);
	}
	
	{
	QPushButton* btn = new QPushButton("Strip", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::dehumanise);
	hbox->addWidget(btn);
	}
	
	{
	QPushButton* btn = new QPushButton("Load", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::load_file);
	hbox->addWidget(btn);
	}
	
	{
	QPushButton* btn = new QPushButton("Save", this);
	connect(btn, &QPushButton::clicked, this, &RegexEditor::save_to_file);
	hbox->addWidget(btn);
	}
	

	l->addLayout(hbox);
	}
	
	{
		QHBoxLayout* hbox = new QHBoxLayout;
		{
			QPushButton* btn = new QPushButton("Partner", this);
			connect(btn, &QPushButton::clicked, this->text_editor, &CodeEditor::jump_to_partner);
			hbox->addWidget(btn);
		}
		l->addLayout(hbox);
	}
	
	this->setLayout(l);
}

void RegexEditor::find_text(){
	SQLNameDialog* dialog = new SQLNameDialog("Find"); // TODO: Have PatternNameDialog, perhaps which SQLNameDialog inherits. SQLNameDialog also needs a case-insensitive option.
	const int rc = dialog->exec();
	const char* patternstr;
	dialog->interpret(patternstr);
	const bool is_pattern = !(patternstr[0] == '=');
	QString qstr = dialog->name_edit->text();
	
	delete dialog;
	
	if (rc != QDialog::Accepted  ||  qstr.isEmpty())
		return;
	
	const int current_pos = this->text_editor->textCursor().position();
	
	if (!is_pattern)
		qstr = QRegularExpression::escape(qstr);
	const QRegularExpression expr(qstr);
	
	QRegularExpressionMatch match = expr.match(this->text_editor->toPlainText(), current_pos);
	
	if (!match.hasMatch()){
		if (current_pos != 0){
			// If no match found, try again from the start of the document
			match = expr.match(this->text_editor->toPlainText(), 0);
		}
		if (!match.hasMatch()){
			QMessageBox::information(this,  "Pattern not found",  qstr);
			return;
		}
	}
	
	QTextCursor cursor = this->text_editor->textCursor();
	cursor.setPosition(match.capturedStart());
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
	this->text_editor->setTextCursor(cursor);
}

void RegexEditor::display_help() const{
	QMessageBox::information(0,  "Help",  help_text);
}



int get_line_n(const QString& s,  int end){
	int n = 1;
	for (auto i = 0;  i <= end;  ++i)
		if (s.at(i) == QChar('\n'))
			++n;
	return n;
}

bool RegexEditor::to_final_format(const bool optimise,  QString& buf,  int i,  int j,  int last_optimised_group_indx,  int var_depth){ // Use seperate buffer to avoid overwriting text_editor contents
	// WARNING: Does not currently support special encodings, i.e. non-ASCII characters are likely to be mangled.
	// TODO: Add utf8 support.
	const QString q = this->text_editor->toPlainText();
	
	int group_start = 0;
	int group_start_offset;
	static std::vector<QStringRef> var_names;
	static std::vector<QStringRef> var_values;
	static std::vector<int> var_starts;
	bool on_line_where_group_was_declared = false;
	bool do_not_optimise_this_group; // Initialised at the start of every group
	for(;  i < q.size();  ){
		QChar c = q.at(i);
		if (c == QChar('\\')){
			// Recognised escapes: \\, \n, \r, \t, \v
			// All others simply become the literal value of the next character
			if (++i == q.size())
				break;
			QChar ch = q.at(i);
			if      (ch == QChar('n'))  ch = QChar('\n');
			else if (ch == QChar('r'))  ch = QChar('\r');
			else if (ch == QChar('t'))  ch = QChar('\t');
			else if (ch == QChar('v'))  ch = QChar('\v'); // Vertical tab
			else if (ch == QChar('\\'));
			else if (ch == QChar('\n'));
			else if (ch == QChar('\t'));
			else if (ch == QChar(' '));
			else if (ch == QChar('{'));
			else if (ch == QChar('}'));
			else if (ch == QChar('('));
			else if (ch == QChar(')'));
			else {
				constexpr static const int ctx = 10;
				MsgBox* msgbox = new MsgBox(
					0,  
					"Unrecognised escape sequence: \\" + QString(ch) + " at line " + QString::number(get_line_n(q, i)),
					QStringRef(&q,  (i >= ctx) ? i - ctx : 0,  (i + ctx < q.size()) ? i + ctx : q.size() - 1).toString()
				);
				msgbox->exec();
				delete msgbox;
				goto goto_RE_tff_cleanup;
			}
			
			buf[j++] = ch;
			++i;
			continue;
		}
		if (c == QChar('$')  &&  q.at(i+1) == QChar('{')){
			i += 2; // Skip ${
			const int substitute_var_name_start = i;
			while(q.at(i++) != QChar('}'));
			const QStringRef substitute_var_name(&q,  substitute_var_name_start,  i - 1 /* backtrack } */ - substitute_var_name_start);
			QStringRef var = nullptr;
			for (size_t k = var_names.size();  k != 0;  ){
				--k;
				if (var_names[k] != substitute_var_name)
					continue;
				var = var_values[k];
			}
			if (var == nullptr){
				// Variable of the given name was not declared before
				QString msg = "Previously defined variables:";
				for (size_t k = var_names.size();  k != 0;  ){
					msg += "\n";
					msg += var_names[--k];
				}
				MsgBox* msgbox = new MsgBox(
					0,  
					"Undeclared variable: " + substitute_var_name + "\nAt line " + QString::number(get_line_n(q, i)),
					msg
				);
				msgbox->exec();
				delete msgbox;
				goto goto_RE_tff_cleanup;
			}
			buf += var;
			j += var.size();
			this->ensure_buf_sized(j);
			continue;
		}
		if (c == QChar('{')){
			if (q.at(i+1) == QChar('?')  &&  q.at(i+2) == QChar('P')  && q.at(i+3) == QChar('<')){
				i += 4;
				const int var_name_start = i;
				while(q.at(i++) != QChar('>'));
				var_names.emplace_back(&q,  var_name_start,  i - 1 /* Backtrack > */ - var_name_start);
				var_values.push_back(nullptr);
				var_starts.push_back(j);
				continue;
			}
		}
		if (c == QChar('}')){
			size_t k = var_values.size();
			while(true){
				if (k == 0){
					QMessageBox::warning(0,  "Unacceptable Syntax",  QString("Line %1: Encountered unescaped '}' without preceding '{?P<VARNAME>' or '${VARNAME'").arg(get_line_n(q, i)));
					goto goto_RE_tff_cleanup;
				}
				if (var_values[--k] == nullptr)
					break;
			}
			var_values[k] = QStringRef(&buf,  var_starts[k],  j - var_starts[k]);
			++i;
			continue;
		}
		if (c == QChar('\n')){
			++i;
			while((i < q.size())  &&  (q.at(i) == QChar(' ')  ||  q.at(i) == QChar('\t')))
				++i;
			on_line_where_group_was_declared = false;
			continue;
		}
		if (c == QChar('#')  &&  (i == 0  ||  q.at(i-1) == QChar(' ')  ||  q.at(i-1) == QChar('\t')  ||  q.at(i-1) == QChar('\n'))){
			do {
				// Remove all preceding unescaped whitespace
				--j;
			} while (
				(j >= 0  && buf.at(j) == QChar(' '))  ||
				(j >= 1  &&  buf.at(j) == QChar('\t')  &&  buf.at(j-1) != QChar('\\'))
			);
			++j;
			
			if (on_line_where_group_was_declared  and  i != 0  and  q.at(i-1) != QChar('\n')){
				if (q.at(i+1) == QChar('F')  and  q.at(i+2) == QChar('L')  and  q.at(i+3) == QChar('A')  and  q.at(i+4) == QChar('G')  and  q.at(i+5) == QChar('=')){
					i += 6;
					int _end_of_flag = i;
					while(i < q.size()  and  q.at(_end_of_flag) != QChar(' ')  and q.at(_end_of_flag) != QChar('\t')  and q.at(_end_of_flag) != QChar('\n'))
						++_end_of_flag;
					if (optimise  and  q.at(i) == QChar('N')  and  q.at(i+1) == QChar('o')  and  q.at(i+2) == QChar('O')  and  q.at(i+3) == QChar('p')  and  q.at(i+4) == QChar('t')){
						do_not_optimise_this_group = true;
					} else {
						MsgBox* msgbox = new MsgBox(
							0,  
							"Unrecognised flag: " + QStringRef(&q,  i,  _end_of_flag - i) + " at line " + QString::number(get_line_n(q, i)),
							"Recognised flags:\n"
							"	NoOpt"
						);
						msgbox->exec();
						delete msgbox;
						goto goto_RE_tff_cleanup;
					}
				}
			}
			
			++i;
			while((i < q.size())  &&  (q.at(i) != QChar('\n')))
				++i;
			continue;
		}
		if (optimise){
			if (c == QChar('(')  &&  j != last_optimised_group_indx){ // Minimum offset for non-trivial group: (ab|c)
				group_start_offset = 0;
				if (q.at(i+1) == QChar('?')  &&  q.at(i+2) == QChar(':'))
					group_start_offset += 3;
				else if (q.at(i+1) == QChar('?')  &&  q.at(i+2) == QChar('P')  &&  q.at(i+3) == QChar('<')){
					group_start_offset += 4;
					while(q.at(i+group_start_offset) != QChar('>')){
						++group_start_offset;
					}
					++group_start_offset;
				} else group_start_offset += 1;
				on_line_where_group_was_declared = true; // To allow capture group flags - such as #DoNotOptimise - to be declared inline with the group declaration, as a comment
				do_not_optimise_this_group = false;
				group_start = j;
			} else if (c == QChar(')')  and  group_start != 0  and not do_not_optimise_this_group){
				const int group_start_actual = group_start + group_start_offset;
				const QStringRef group_text(&buf,  group_start_actual,  j - group_start_actual);
				QString group_replacement;
				const QString group_str = group_text.toString();
				QString s = group_str;
				optimise_regex(s, group_replacement);
				buf.replace(group_start_actual,  j - group_start_actual,  group_replacement);
				return to_final_format(optimise,  buf,  i,  group_start_actual + group_replacement.size(),  group_start,  var_depth);
			}
		}
		
		buf[j++] = c;
		
		++i;
	}
	buf.resize(j); // Strips excess space, as buf is guaranteed to be smaller than q (until variable substitution is implemented)
	
	var_names.clear();
	var_values.clear();
	var_starts.clear();
	
	return true;

	goto_RE_tff_cleanup:
	var_names.clear();
	var_values.clear();
	var_starts.clear();
	return false;
}

void RegexEditor::test_regex(){
	QString buf; // Dummy character to create space for 1 char at beginning
	buf.reserve(this->text_editor->toPlainText().size());
	if (!this->to_final_format(this->does_user_want_optimisations(), buf, 0, 0))
		return;
	
	QByteArray ba = buf.toLocal8Bit();
	char* const s = ba.data();
	
	printf("[%lu] %s\n", strlen(s), s);
	
	std::vector<char*> reason_name2id = {"None", "Unspecified"};
	std::vector<int> groupindx2reason;
	std::vector<char*> group_starts;
	std::vector<char*> group_ends;
	std::vector<bool> record_contents;
	
	compsky::regex::convert_named_groups(s,  s,  reason_name2id,  groupindx2reason, record_contents, group_starts, group_ends);
	printf("%s\n", s);
	
	try {
		auto r = new boost::basic_regex<char, boost::cpp_regex_traits<char>>(s,  boost::regex::perl);
		delete r;
	} catch (boost::regex_error& e){
		MsgBox* msgbox = new MsgBox(0, e.what(), s, 720);
		msgbox->exec();
		delete msgbox;
		return;
	}
	
	QString report = "";

	bool try_exrex = true;
	report += QString::number(group_starts.size() - 1);
	report += " Capture Groups:";
	for (size_t i = 1;  i < groupindx2reason.size();  ++i){
		report += "\n";
		report += QString::number(i);
		report += "\t";
		report += (record_contents[i]) ? "[Record contents]" : "[Count occurances]";
		report += "\t";
		report += reason_name2id[groupindx2reason[i]];
		report += "\n\t";

		// WARNING: The following calculates the number of bytes, NOT the number of characters. QStrings are UTF-16, so some characters are multiple bytes. This is also why QStringRef is not used shortly after.
		const int group_source_strlen = (uintptr_t)(group_ends[i]) - (uintptr_t)(group_starts[i]) - 1;

		if (group_ends[i] == nullptr){
			QMessageBox::critical(0,  "Bug",  QString("Cannot locate group %1's end.\nGroup begins at the %2th position in the final (processed) regex.\nGroup may begin around line %3.").arg(i).arg(groupindx2reason[i]).arg(get_line_n(this->text_editor->toPlainText(), groupindx2reason[i])));
			return;
		}
		const char c = group_ends[i][0];
		group_ends[i][0] = 0;
		QString group_source = group_starts[i];
		group_ends[i][0] = c;
		group_source.resize(group_source_strlen);

		// TODO: Optionally truncate large sources

		report += group_source;
		
		if (try_exrex){
			QProcess exrex;
			QString output;
			
			exrex.start("exrex.py",  {"-r", "-m", "1", group_source});
			
			if (!(try_exrex = exrex.waitForFinished()))
				continue;
			
			report += "\neg:\t";
			report += exrex.readAllStandardOutput();
			
			exrex.close();
		}
	}
	
	if (!try_exrex)
		report += "\n\nTo see example strings that match each group regex, pip install exrex";
	
	MsgBox* msgbox = new MsgBox(0, "Success", report, 720);
	msgbox->exec();
	delete msgbox;
}

bool RegexEditor::does_user_want_optimisations() const {
	return this->want_optimisations->isChecked();
}


void RegexEditor::dehumanise(){
	QString buf;
	const int buf_sz = this->text_editor->toPlainText().size() + 1; // Extra char for trailing \0
	buf.reserve(buf_sz);
	
	this->ensure_buf_sized(buf_sz);
	
	if (!this->to_final_format(this->does_user_want_optimisations(), buf, 0, 0))
		return;
	
	MsgBox* const msgbox = new MsgBox(this, "Dehumanised Form", buf, 720);
	msgbox->exec();
	delete msgbox;
}


void RegexEditor::set_text(const QString& str){
	this->text_editor->setPlainText(str);
}


QString RegexEditor::get_text() const {
	return this->text_editor->toPlainText();
}


void RegexEditor::load_file(){
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	if (!dialog.exec())
		return;
	
	QString const file_path = dialog.selectedFiles()[0];
	
	QFile f(file_path);
	if (!f.open(QIODevice::ReadOnly)){
		QMessageBox::information(0, "Cannot open file", f.errorString());
		return;
	}
	
	QTextStream in(&f);
	
	QString content = "";
	while(!in.atEnd()) {
		content += in.readLine() + "\n";
	}
	
	f.close();
	
	this->text_editor->setPlainText(content);
}


void RegexEditor::save_to_file(){
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	if (!dialog.exec())
		return;
	
	QString const file_path = dialog.selectedFiles()[0];
	
	QFile f(file_path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)){
		QMessageBox::information(0, "Cannot open file for writing", f.errorString());
		return;
	}
	
	QTextStream out(&f);
	out << this->text_editor->toPlainText();
	f.close();
}

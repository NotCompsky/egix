/*
 * rscraper Copyright (C) 2019 Adam Gray
 * This program is licensed with GPLv3.0 and comes with absolutely no warranty.
 * This code may be copied, modified, distributed etc. with accordance to the GPLv3.0 license (a copy of which is in the root project directory) under the following conditions:
 *     This copyright notice must be included at the beginning of any copied/modified file originating from this project, or at the beginning of any section of code that originates from this project.
 */

#ifndef RSCRAPER_HUB_REGEX_EDITOR_HPP
#define RSCRAPER_HUB_REGEX_EDITOR_HPP

#include <QCheckBox>
#include <QDialog>


class CodeEditor;


class RegexEditor : public QDialog {
  public:
	RegexEditor(QWidget* parent = nullptr);
  protected Q_SLOTS:
	void test_regex();
	void dehumanise();
	virtual void load_file();
	virtual void save_to_file();
	void set_text(const QString& str);
	QString get_text() const;
  protected:
	void find_text();
	void ensure_buf_sized(const size_t buf_sz);
	bool does_user_want_optimisations() const;
	char* buf;
	char* itr;
	int buf_sz; // int, rather than size_t, because that is what Qt uses
	bool to_final_format(const bool optimise,  QString& buf,  int i = 0,  int j = 1,  int last_optimised_group_indx = 0,  int var_depth = 0);
	void display_help() const;
	QCheckBox* want_optimisations;
	CodeEditor* text_editor;
};


#endif

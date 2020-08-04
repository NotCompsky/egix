<p align="center">
	<h1 align="center">egix</h1>
</p>

<p align="center">
	<a href="LICENSE"><img src="https://img.shields.io/github/license/NotCompsky/egix"/></a>
	<a href="https://github.com/NotCompsky/egix/releases"><img src="https://img.shields.io/github/v/release/NotCompsky/egix"/></a>
	<a href="https://hub.docker.com/repository/docker/NotCompsky/egix/tags"><img src="https://img.shields.io/docker/image-size/NotCompsky/egix?label=Docker%20image"/></a>
	<a href="https://circleci.com/gh/NotCompsky/egix"><img src="https://circleci.com/gh/NotCompsky/egix.svg?style=shield"/></a>
	<a href="https://github.com/NotCompsky/egix/graphs/commit-activity"><img src="https://img.shields.io/github/commit-activity/w/NotCompsky/egix"/>
	<a href="https://github.com/NotCompsky/egix/graphs/contributors"><img src="https://img.shields.io/github/contributors/NotCompsky/egix"></a>
</p>

![screenshot1](https://user-images.githubusercontent.com/30552567/89293819-7368c900-d656-11ea-85d6-f1dea3c262ac.png)

A Qt-based Regex Editor

### Features

* Inline comments
* Syntax Highlighting
* Jump to matching brackets
* Interoperability with [libcompsky](https://github.com/NotCompsky/libcompsky)'s regex manipulator - which would allow for almost arbitrary capture group names, and having multiple capture groups of the same name.

### Used By

* [rscraper](https://github.com/NotCompsky/rscraper)

### FAQ

#### What does egix stand for?

I have forgotten why it is called `egix`.

### Background

Developed as part of the [rscraper](https://github.com/NotCompsky/rscraper) project's `hub`, split off when I realised I was often opening `rscraper-hub` solely to use its regex editor.

#pragma once

#include <QJsonDocument>
#include <QMainWindow>

class QJsonDocument;
class QSplitter;
class QStatusBar;
class QTabWidget;
class QTextEdit;
class QToolBar;

namespace valijson {
    class Schema;
}

class Window : public QMainWindow
{
    Q_OBJECT

public:
    explicit Window(QWidget * parent = 0);

public slots:
    void refreshJson();

    void showOpenDocumentDialog();
    void showOpenSchemaDialog();

private:
    QTextEdit * createEditor(bool readOnly);
    QSplitter * createSplitter(QWidget * left, QWidget * right, bool horizontal);
    QStatusBar * createStatusBar();
    QTabWidget * createTabWidget(QWidget * child, const QString & name);
    QToolBar * createToolBar();

    void validate();

    QTextEdit * m_documentEditor;
    QTextEdit * m_schemaEditor;

    QTextEdit * m_errors;

    QJsonDocument m_documentJson;
    QJsonDocument m_schemaJson;

    valijson::Schema * m_schema;
};

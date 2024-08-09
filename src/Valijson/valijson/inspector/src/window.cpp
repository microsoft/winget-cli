#include <sstream>
#include <stdexcept>

#include <QFile>
#include <QFileDialog>
#include <QMenu>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QTabWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>

#include <valijson/adapters/qtjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#include "highlighter.h"
#include "window.h"

Window::Window(QWidget * parent)
  : QMainWindow(parent)
  , m_schema(nullptr)
{
    setWindowTitle("JSON Inspector");

    m_documentEditor = createEditor(false);
    m_schemaEditor = createEditor(false);
    m_errors = createEditor(true);

    auto documentTabWidget = createTabWidget(m_documentEditor, "Document");
    auto schemaTabWidget = createTabWidget(m_schemaEditor, "Schema");
    auto horizontalSplitter = createSplitter(schemaTabWidget, documentTabWidget, true);

    auto errorsTabWidget = createTabWidget(m_errors, "Errors");
    auto verticalSplitter = createSplitter(horizontalSplitter, errorsTabWidget, false);
    verticalSplitter->setStretchFactor(0, 2);
    verticalSplitter->setStretchFactor(1, 1);

    auto toolBar = createToolBar();
    auto statusBar = createStatusBar();

    addToolBar(toolBar);
    setCentralWidget(verticalSplitter);
    setStatusBar(statusBar);

    connect(m_documentEditor, SIGNAL(textChanged()), this, SLOT(refreshJson()));
    connect(m_schemaEditor, SIGNAL(textChanged()), this, SLOT(refreshJson()));

    refreshJson();
}

QTextEdit * Window::createEditor(bool readOnly)
{
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(12);

    auto editor = new QTextEdit();
    editor->setFont(font);
    editor->setReadOnly(readOnly);

    auto highlighter = new Highlighter(editor->document());

    return editor;
}

QSplitter * Window::createSplitter(QWidget * left, QWidget * right, bool horizontal)
{
    auto splitter = new QSplitter(horizontal ? Qt::Horizontal : Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->insertWidget(0, left);
    splitter->insertWidget(1, right);

    return splitter;
}

QStatusBar * Window::createStatusBar()
{
    return new QStatusBar();
}

QTabWidget * Window::createTabWidget(QWidget * child, const QString & name)
{
    auto tabWidget = new QTabWidget();
    tabWidget->addTab(child, name);
    tabWidget->setDocumentMode(true);

    return tabWidget;
}

QToolBar * Window::createToolBar()
{
    auto toolbar = new QToolBar();
    toolbar->setMovable(false);

    auto openMenu = new QMenu("Open");
    auto openSchemaAction = openMenu->addAction("Open Schema...");
    auto openDocumentAction = openMenu->addAction("Open Document...");

    auto openButton = new QToolButton();
    openButton->setMenu(openMenu);
    openButton->setPopupMode(QToolButton::MenuButtonPopup);
    openButton->setText("Open");
    openButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolbar->addWidget(openButton);

    connect(openButton, &QToolButton::clicked, openButton, &QToolButton::showMenu);
    connect(openDocumentAction, SIGNAL(triggered()), this, SLOT(showOpenDocumentDialog()));
    connect(openSchemaAction, SIGNAL(triggered()), this, SLOT(showOpenSchemaDialog()));

    return toolbar;
}

void Window::refreshJson()
{
    QString errors;
    m_errors->setText("");

    const auto schema = m_schemaEditor->toPlainText().toUtf8();
    const auto doc = m_documentEditor->toPlainText().toUtf8();

    if (schema.isEmpty()) {
        if (doc.isEmpty()) {
            m_errors->setText(
              "Please provide a schema and a document to be validated.\n\n"
              "Note that this example uses QtJson, which does not consider non-array and "
              "non-object values to be valid JSON documents.");
            return;
        } else {
            errors += "Schema error: must not be empty\n\n";
        }
    } else {
        QJsonParseError error;
        m_schemaJson = QJsonDocument::fromJson(schema, &error);
        if (m_schemaJson.isNull()) {
            errors += QString("Schema error: ") + error.errorString() + "\n\n";
        }
    }

    if (doc.isEmpty()) {
        if (!schema.isEmpty()) {
            errors += "Document error: must not be empty\n\n";
        }
    } else {
        QJsonParseError error;
        m_documentJson = QJsonDocument::fromJson(doc, &error);
        if (m_documentJson.isNull()) {
            errors += QString("Document error: ") + error.errorString() + "\n\n";
        }
    }

    if (!errors.isEmpty()) {
      m_errors->setText(errors);
      return;
    }

    try {
        valijson::adapters::QtJsonAdapter adapter(m_schemaJson.object());
        valijson::SchemaParser parser;
        delete m_schema;
        m_schema = new valijson::Schema();
        parser.populateSchema(adapter, *m_schema);
        validate();
    } catch (std::runtime_error & error) {
        delete m_schema;
        m_schema = nullptr;
        m_errors->setText(QString("Schema error: ") + error.what());
    }
}

void Window::showOpenDocumentDialog()
{
    const QString fileName = QFileDialog::getOpenFileName(this, "Open Document", QString(), QString("*.json"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        file.open(QFile::ReadOnly | QFile::Text);
        m_documentEditor->setText(file.readAll());
    }
}

void Window::showOpenSchemaDialog()
{
    const QString fileName = QFileDialog::getOpenFileName(this, "Open Schema", QString(), QString("*.json"));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        file.open(QFile::ReadOnly | QFile::Text);
        m_schemaEditor->setText(file.readAll());
    }
}

void Window::validate()
{
    valijson::ValidationResults results;
    valijson::Validator validator;
    valijson::adapters::QtJsonAdapter adapter(m_documentJson.object());

    if (validator.validate(*m_schema, adapter, &results)) {
        m_errors->setText("Document is valid.");
        return;
    }

    valijson::ValidationResults::Error error;
    unsigned int errorNum = 1;
    std::stringstream ss;
    while (results.popError(error)) {
        std::string context;
        for (auto & itr : error.context) {
            context += itr;
        }

        ss << "Validation error #" << errorNum << std::endl
            << "  context: " << context << std::endl
            << "  desc:    " << error.description << std::endl;
        ++errorNum;
    }

    m_errors->setText(QString::fromStdString(ss.str()));
}
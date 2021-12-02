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

    connect(m_documentEditor, SIGNAL(textChanged()), this, SLOT(refreshDocumentJson()));
    connect(m_schemaEditor, SIGNAL(textChanged()), this, SLOT(refreshSchemaJson()));
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

void Window::refreshDocumentJson()
{
    QJsonParseError error;
    m_document = QJsonDocument::fromJson(m_documentEditor->toPlainText().toUtf8(), &error);
    if (m_document.isNull()) {
        m_errors->setText(error.errorString());
        return;
    }

    if (m_schema) {
        validate();
    } else {
        m_errors->setText("");
    }
}

void Window::refreshSchemaJson()
{
    QJsonParseError error;
    auto schemaDoc = QJsonDocument::fromJson(m_schemaEditor->toPlainText().toUtf8(), &error);
    if (schemaDoc.isNull()) {
        m_errors->setText(error.errorString());
        return;
    }

    try {
        valijson::adapters::QtJsonAdapter adapter(schemaDoc.object());
        valijson::SchemaParser parser;
        delete m_schema;
        m_schema = new valijson::Schema();
        parser.populateSchema(adapter, *m_schema);
        m_errors->setText("");

    } catch (std::runtime_error error) {
        delete m_schema;
        m_schema = nullptr;
        m_errors->setText(error.what());
    }

    validate();
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
    valijson::adapters::QtJsonAdapter adapter(m_document.object());

    if (validator.validate(*m_schema, adapter, &results)) {
        m_errors->setText("Document is valid.");
        return;
    }

    valijson::ValidationResults::Error error;
    unsigned int errorNum = 1;
    std::stringstream ss;
    while (results.popError(error)) {
        std::string context;
        for (auto itr = error.context.begin(); itr != error.context.end(); itr++) {
            context += *itr;
        }

        ss << "Error #" << errorNum << std::endl
            << "  context: " << context << std::endl
            << "  desc:    " << error.description << std::endl;
        ++errorNum;
    }

    m_errors->setText(QString::fromStdString(ss.str()));
}
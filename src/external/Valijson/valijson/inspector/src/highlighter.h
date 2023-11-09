#pragma once

#include <QSyntaxHighlighter>

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    Highlighter(QTextDocument * parent = 0);

protected:
    void highlightBlock(const QString & text) override;
};

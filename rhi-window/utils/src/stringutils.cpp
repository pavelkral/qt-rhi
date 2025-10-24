#include "stringutils.h"

#include <QRegularExpression>

StringUtils::StringUtils(QObject* parent) : QObject{ parent } {}

QString StringUtils::mdToHtml( QString& S) {

    QString html = S;

    // --- 1. Code blocks (```language\ncode```) ---
    static const QRegularExpression codeBlockRegExp(
        "```\\s*(\\S*)?\n(.*?)```",
        QRegularExpression::DotMatchesEverythingOption
    );

    QRegularExpressionMatchIterator codeIter = codeBlockRegExp.globalMatch(html);
    while (codeIter.hasNext()) {
        QRegularExpressionMatch match = codeIter.next();
        QString fullMatch = match.captured(0);       // Full match (```...```)
        QString language = match.captured(1);       // Language hint (e.g. cpp)
        QString code = match.captured(2);       // Code content

        QString classAttr;
        if (!language.isEmpty()) {
            classAttr = QString(" class=\"language-%1\"").arg(language);
        }

        QString replacement = QString("<pre><code%1>%2</code></pre>")
            .arg(classAttr, code.toHtmlEscaped());

        html.replace(fullMatch, replacement);
    }

    // --- 2. Inline code (`code`) ---
    // This must run *after* the multiline code blocks!
    static const QRegularExpression inlineCodeRegExp("`(.*?)`");
    html.replace(inlineCodeRegExp, "<code>\\1</code>");

    // --- 3. Headers ---
    // ### Header → <h3>, ## Header → <h2>, # Header → <h1>
    html.replace(
        QRegularExpression("^###\\s*(.*)$", QRegularExpression::MultilineOption),
        "<h3>\\1</h3>"
    );
    html.replace(
        QRegularExpression("^##\\s*(.*)$", QRegularExpression::MultilineOption),
        "<h2>\\1</h2>"
    );
    html.replace(
        QRegularExpression("^#\\s*(.*)$", QRegularExpression::MultilineOption),
        "<h1>\\1</h1>"
    );

    // --- 4. Bold text ---
    // **text** or __text__ → <strong>
    html.replace(
        QRegularExpression("\\*\\*(.*?)\\*\\*|__(.*?)__"),
        "<strong>\\1\\2</strong>"
    );

    // --- 5. Italic text ---
    // *text* or _text_ → <em>
    html.replace(
        QRegularExpression("(?<!\\*)\\*(?!\\*)(.*?)(?<!\\*)\\*(?!\\*)|(?<!_)(?!__)_(.*?)(?<!_)_(?!__)"),
        "<em>\\1\\2</em>"
    );

    // --- 6. Hyperlinks ---
    // [text](url) → <a href="url">text</a>
    html.replace(
        QRegularExpression("\\[(.*?)\\]\\((.*?)\\)"),
        "<a href=\"\\2\">\\1</a>"
    );

    // --- 7. Unordered list ---
    // Lines starting with - or * → <ul><li>...</li></ul>
    QStringList listItems;
    static const QRegularExpression listItemRegExp(
        "^\\s*[-*]\\s*(.*)$",
        QRegularExpression::MultilineOption
    );

    QRegularExpressionMatchIterator listIter = listItemRegExp.globalMatch(html);
    while (listIter.hasNext()) {
        QRegularExpressionMatch match = listIter.next();
        listItems << "<li>" + match.captured(1).trimmed() + "</li>";
    }

    if (!listItems.isEmpty()) {
        QString ul = "<ul>\n" + listItems.join("\n") + "\n</ul>";
        html.replace(listItemRegExp, ""); // Remove original list lines
        html.prepend(ul + "\n");
    }

    // --- 8. Line breaks ---
    // Convert remaining newlines to <br>
    html.replace("\n", "<br>");
    return html;

}

QString StringUtils::addHtmlStyle(const QString& str) {

	// QMessageBox::information(this, "info"," tags ");
    qDebug() << "adding headers";

    QString html_str = str;
    // st.replace(QString("\n"), QString("\n<br />"));
    // st.prepend(
    //     "<html>\n"
    //     "<head>\n"
    //     "<title>title</title>\n"
    //     "<style>\n"
    //     "  body { background-color: #fdf6e3; color: #002b36; font-family: 'Courier New'; font-size: 14px; }\n"
    //     "  h1 { color: #b58900; font-size: 20px; margin-bottom: 10px; }\n"
    //     "  p { margin: 0 0 10px 0; }\n"
    //     "  code { background-color: #eee8d5; padding: 2px 4px; border-radius: 4px; font-family: monospace; }\n"
    //     "</style>\n"
    //     "</head>\n"
    //     "<body>\n\n"
    //     );

    html_str.prepend(
        "<html>\n"
        "<head>\n"
        "<title>GitHub Markdown Style</title>\n"
        "<meta charset=\"utf-8\">\n"
        "<style>\n"
        "  body {\n"
        "    background: white;\n"
        "    color: #24292e;\n"
        "    font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Helvetica, Arial, sans-serif, \"Apple Color Emoji\", \"Segoe UI Emoji\";\n"
        "    font-size: 16px;\n"
        "    line-height: 1.5;\n"
        "    padding: 20px;\n"
        "  }\n"
        "  h1, h2, h3, h4, h5, h6 {\n"
        "    font-weight: 600;\n"
        "    margin-top: 24px;\n"
        "    margin-bottom: 16px;\n"
        "    border-bottom: 1px solid #eaecef;\n"
        "    padding-bottom: .3em;\n"
        "  }\n"
        "  p {\n"
        "    margin-top: 0;\n"
        "    margin-bottom: 16px;\n"
        "  }\n"
        "  code {\n"
        "    background-color: rgba(27,31,35,0.05);\n"
        "    padding: .2em .4em;\n"
        "    margin: 0;\n"
        "    font-size: 85%;\n"
        "    font-family: SFMono-Regular, Consolas, Liberation Mono, Menlo, monospace;\n"
        "    border-radius: 6px;\n"
        "  }\n"
        "  pre {\n"
        "    background-color: #f6f8fa;\n"
        "    padding: 16px;\n"
        "    overflow: auto;\n"
        "    font-size: 85%;\n"
        "    line-height: 1.45;\n"
        "    border-radius: 6px;\n"
        "  }\n"
        "  blockquote {\n"
        "    padding: 0 1em;\n"
        "    color: #6a737d;\n"
        "    border-left: .25em solid #dfe2e5;\n"
        "  }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n\n"
        );

    html_str.append("\n</body>\n</html>");
    return html_str;
}

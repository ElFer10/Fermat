#pragma once

#include <QFont>
#include <QFontMetricsF>
#include <QString>
#include <QtGlobal>

struct MathStyle
{
    QFont baseFont;
    bool italicVariables = true;

    QFontMetricsF metrics() const
    {
        return QFontMetricsF(baseFont);
    }

    QFont scaledFont(qreal factor) const
    {
        QFont result = baseFont;

        if (result.pointSizeF() > 0.0)
        {
            result.setPointSizeF(qMax(1.0, result.pointSizeF() * factor));
        }
        else if (result.pixelSize() > 0)
        {
            result.setPixelSize(qMax(1, qRound(result.pixelSize() * factor)));
        }

        return result;
    }

    QFont scriptFont() const
    {
        return scaledFont(0.70);
    }

    QFont limitFont() const
    {
        return scaledFont(0.65);
    }

    QFont largeOperatorFont() const
    {
        return scaledFont(1.55);
    }

    bool isVariable(const QString &text) const
    {
        /*
         * Actualmente cada TextNode suele representar
         * un carácter. Consideramos variable una única
         * letra, incluyendo letras griegas Unicode.
         *
         * Los números, operadores y signos permanecen
         * en estilo normal.
         */
        return text.size() == 1 && text.front().isLetter();
    }

    QFont textFont(const QString &text) const
    {
        QFont result = baseFont;

        result.setItalic(italicVariables && isVariable(text));

        return result;
    }

    MathStyle scaled(qreal factor) const
    {
        MathStyle result = *this;
        result.baseFont = scaledFont(factor);
        return result;
    }
};

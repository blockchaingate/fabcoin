// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FABCOIN_QT_FABCOINADDRESSVALIDATOR_H
#define FABCOIN_QT_FABCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class FabcoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FabcoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Fabcoin address widget validator, checks for a valid fabcoin address.
 */
class FabcoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit FabcoinAddressCheckValidator(QObject *parent, bool allowScript = true);

    State validate(QString &input, int &pos) const;

private:
    bool bAllowScript;
};

#endif // FABCOIN_QT_FABCOINADDRESSVALIDATOR_H

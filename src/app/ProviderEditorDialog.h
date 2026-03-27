#pragma once

#include "core/Models.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace llm {

class ProviderEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProviderEditorDialog(QWidget* parent = nullptr);

    ProviderId selectedProvider() const;
    QString displayName() const;

private:
    QComboBox* m_providerCombo = nullptr;
    QLineEdit* m_nameEdit = nullptr;
};

}  // namespace llm

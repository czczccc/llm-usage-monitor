#include "ProviderEditorDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QVBoxLayout>

namespace llm {

ProviderEditorDialog::ProviderEditorDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("Add provider"));

    auto* rootLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();
    m_providerCombo = new QComboBox(this);
    for (ProviderId providerId : allProviderIds()) {
        m_providerCombo->addItem(providerDisplayName(providerId), providerIdToString(providerId));
    }
    m_nameEdit = new QLineEdit(this);

    formLayout->addRow(QStringLiteral("Provider"), m_providerCombo);
    formLayout->addRow(QStringLiteral("Display name"), m_nameEdit);
    rootLayout->addLayout(formLayout);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    rootLayout->addWidget(buttons);
}

ProviderId ProviderEditorDialog::selectedProvider() const {
    return providerIdFromString(m_providerCombo->currentData().toString());
}

QString ProviderEditorDialog::displayName() const {
    return m_nameEdit->text().trimmed();
}

}  // namespace llm

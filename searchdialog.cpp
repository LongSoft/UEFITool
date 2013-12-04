/* searchdialog.cpp

  Copyright (c) 2013, Nikolaj Schlej. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

*/

#include "searchdialog.h"

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    // Create UI
    ui->setupUi(this);

	// Connect
	//connect(ui->dataTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setEditMask()));
	//connect(ui->translateFromHexCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setEditMask()));
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

/*void SearchDialog::setEditMask()
{
	int index = ui->dataTypeComboBox->currentIndex();
	QString mask;
	if (index == 0) // Hex pattern, max 48 bytes long
		mask = "";
	else if (index == 1) {
		if (ui->translateFromHexCheckBox->isChecked())
			mask = "<HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH;_";
		else
			mask = "<HHHHHHHH-HHHH-HHHH-HHHHHHHHHHHHHHHH;_";
	}
	else
		mask = "";
	ui->searchEdit->setInputMask(mask);
}*/
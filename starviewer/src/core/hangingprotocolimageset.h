/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#ifndef UDGHANGINGPROTOCOLIMAGESET_H
#define UDGHANGINGPROTOCOLIMAGESET_H

#include "hangingprotocolimagesetrestrictionexpression.h"

namespace udg {

class Series;
class Study;
class HangingProtocol;

class HangingProtocolImageSet {

public:
    HangingProtocolImageSet();
    ~HangingProtocolImageSet();

    /// Identificador de l'Image Set
    void setIdentifier(int identifier);

    /// Obtenir l'identificador de l'Image Set
    int getIdentifier() const;

    /// Assignar el hanging protocol al que pertany
    void setHangingProtocol(HangingProtocol *hangingProtocol);

    /// Obtenir el hanging protocol al que pertany
    HangingProtocol* getHangingProtocol() const;

    /// Returns the restriction expression of this image set.
    const HangingProtocolImageSetRestrictionExpression& getRestrictionExpression() const;
    /// Sets the restrction expression of this image set.
    void setRestrictionExpression(const HangingProtocolImageSetRestrictionExpression &restrictionExpression);

    /// Posar el tipus d'element
    void setTypeOfItem(QString);

    /// Obtenir el tipus d'element
    QString getTypeOfItem() const;

    /// Posar el número de la imatge a mostrar
    void setImageToDisplay(int imageNumber);

    /// Obtenir el número de la imatge a mostrar
    int getImageToDisplay() const;

    /// Assignar la sèrie que es representa
    void setSeriesToDisplay(Series *series);

    /// Obtenir la sèrie que es representa
    Series* getSeriesToDisplay() const;

    /// Mètode per mostrar els valors
    void show();

    /// Metode per indicar si es un estudi previ o no
    void setIsPreviousStudy(bool hasPreviousStudy);

    /// Mètode per saber si te previs o no
    bool isPreviousStudy();

    /// Mètode per posar si esta descarregat o no
    void setDownloaded(bool option);

    /// Mètode per saber si està descarregat o no
    bool isDownloaded();

    /// Posa quin és l'estudi previ
    void setPreviousStudyToDisplay(Study *study);

    /// Obte l'estudi previ a mostrar. Pot estar descarregat o no.
    Study* getPreviousStudyToDisplay();

    /// Posa l'ImageSet del qual és previ
    void setPreviousImageSetReference(int imageSetNumber);

    /// Obté l'ImageSet del qual és previ
    int getPreviousImageSetReference();

    /// Obté l'índex de la imatge a mostrar dins la serie
    int getImageNumberInPatientModality();

    /// Posa l'índex de la imatge a mostar del pacient
    void setImageNumberInPatientModality(int imageNumberInPatientModality);

private:
    /// The restriction expression that this image set must satisfy.
    HangingProtocolImageSetRestrictionExpression m_restrictionExpression;

    /// Identificador únic de l'Image Set
    int m_identifier;

    /// Hanging Protocol al que pertany
    HangingProtocol *m_hangingProtocol;

    /// Per saber si s'ha de tractar a nivell d'imatge o de sèrie.
    QString m_typeOfItem;

    /// Serie que s'ajusta a les restriccions
    Series *m_serieToDisplay;

    /// Número d'imatge a mostrar, es coneix aquest número un cop
    /// trobada la imatge que compleix les restriccions
    int m_imageToDisplay;

    /// Índex de la imatge dins l'estudi. Es coneix aquest número
    /// ja quan es crea el hanging protocol
    int m_imageNumberInPatientModality;

    /// Indica si conte un estudi previ
    bool m_isPreviousStudy;

    /// Indica si esta o no descarregat
    bool m_downloaded;

    /// Estudi previ a l'image set, sense descarregar
    Study *m_previousStudyToDisplay;

    /// Posa l'image set que té de referencia i del qual ha de ser previ
    int m_previousImageSetReference;

};

}

#endif

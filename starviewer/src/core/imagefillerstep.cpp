/***************************************************************************
 *   Copyright (C) 2005-2006 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "imagefillerstep.h"
#include "logging.h"
#include "patientfillerinput.h"
#include "dicomtagreader.h"
#include "patient.h"
#include "study.h"
#include "series.h"
#include "image.h"

#include <cmath> // pel fabs
#include <QApplication> //Per el process events, TODO Treure i fer amb threads.

namespace udg {

ImageFillerStep::ImageFillerStep()
 : PatientFillerStep()
{
    m_requiredLabelsList << "DICOMFileClassifierFillerStep";
}

ImageFillerStep::~ImageFillerStep()
{
}

bool ImageFillerStep::fill()
{
    bool ok = false;
    // processarem cadascun dels pacients que hi hagi en l'input i per cadascun totes les sèries que siguin de tipus imatge
    if( m_input )
    {
        unsigned int i = 0;
        while( i < m_input->getNumberOfPatients() )
        {
            Patient *patient = m_input->getPatient( i );
            this->processPatient( patient );
            i++;
        }
    }
    else
    {
        DEBUG_LOG("No tenim input!");
    }

    return ok;
}

void ImageFillerStep::processPatient( Patient *patient )
{
    QList<Study *> studyList = patient->getStudies();
    foreach( Study *study, studyList )
    {
        QList<Series *> seriesList = study->getSeries();
        foreach( Series *series, seriesList )
        {
            this->processSeries( series );
        }
    }
}

void ImageFillerStep::processSeries( Series *series )
{
    // Podrem tenir o bé Images, o bé KINs o bé PresentationStates
    if( isImageSeries(series) )
    {
        bool ok = false;
        foreach (QString file, series->getFilesPathList())
        {
            Image *image = new Image;
            image->setPath( file );

            if( processImage( image ) )
            {
                ok = true;
                series->addImage( image );
            }
            else
                DEBUG_LOG( "L'arxiu [" + file + "] no es pot afegir com a imatge a la sèrie amb UID [" + series->getInstanceUID() + "]" );

            qApp->processEvents();
        }
        if( ok )
            m_input->addLabelToSeries("ImageFillerStep", series );
        else
            DEBUG_LOG( "La sèrie amb UID [" + series->getInstanceUID() + "] no té cap arxiu que sigui una imatge. No s'etiquetarà amb l'ImageFillerStep." );
    }
    else
    {
        DEBUG_LOG("La serie amb uid " + series->getInstanceUID() + " no es processa perquè no és una sèrie d'Imatges. És de modalitat: " + series->getModality() );
    }
}

bool ImageFillerStep::processImage( Image *image )
{
    DICOMTagReader dicomReader;
    bool ok = dicomReader.setFile( image->getPath() );
    if( !ok )
    {
        DEBUG_LOG("No s'ha pogut obrir amb el tagReader l'arxiu: " + image->getPath() );
        return false;
    }

    // comprovem si l'arxiu és una imatge, per això caldrà que existeixi el tag PixelData
    ok = dicomReader.tagExists( DCM_PixelData );
    if( ok )
    {
        image->setSOPInstanceUID( dicomReader.getAttributeByName( DCM_SOPInstanceUID ) );
        image->setInstanceNumber( dicomReader.getAttributeByName( DCM_InstanceNumber ) );

        QString value = dicomReader.getAttributeByName( DCM_ContentDate );
        if( !value.isEmpty() )
            image->setContentDate(value);

        value = dicomReader.getAttributeByName( DCM_ContentTime );
        if( !value.isEmpty() )
            image->setContentTime(value);

        // \TODO Txapussa per sortir del pas. Serveix per calcular correctament el PixelSpacing
        QString modality = dicomReader.getAttributeByName( DCM_Modality );
        if ( modality == "CT" || modality == "MR")
        {
            value = dicomReader.getAttributeByName( DCM_PixelSpacing );
        }
        else
        {
            value = dicomReader.getAttributeByName( DCM_ImagerPixelSpacing );
        }
        QStringList list;
        if ( !value.isEmpty() )
        {
            list = value.split( "\\" );
            if( list.size() == 2 )
                image->setPixelSpacing( list.at(0).toDouble(), list.at(1).toDouble() );
            else
                DEBUG_LOG("No s'ha trobat cap valor de pixel spacing definit de forma estàndar esperada. Madalitat de la imatge: [" + modality + "]" );
        }

        value = dicomReader.getAttributeByName( DCM_SliceThickness );
        if( !value.isEmpty() )
            image->setSliceThickness( value.toDouble() );

        value = dicomReader.getAttributeByName( DCM_ImageOrientationPatient );
        list = value.split( "\\" );
        if( list.size() == 6 )
        {
            double orientation[6];
            for( int i = 0; i < 6; i++ )
            {
                orientation[ i ] = list.at( i ).toDouble();
            }
            image->setImageOrientationPatient( orientation );

            // cerquem l'string amb la orientació del pacient
            value = dicomReader.getAttributeByName( DCM_PatientOrientation );
            if( !value.isEmpty() )
                image->setPatientOrientation( value.replace( QString("\\") , QString(",") ).replace( QString("F") , QString("I") ).replace( QString("H") , QString("I") ) );
            else // si no tenim aquest valor, el calculem a partir dels direction cosines
            {
                // I ara ens disposem a crear l'string amb l'orientació del pacient
                double *orientation = (double *)image->getImageOrientationPatient();
                double dirCosinesX[3], dirCosinesY[3], dirCosinesZ[3];
                for( int i = 0; i < 3; i++ )
                {
                    dirCosinesX[i] = orientation[i];
                    dirCosinesY[i] = orientation[3+i];
                    dirCosinesZ[i] = orientation[6+i];
                }
                QString patientOrientationString;
                // \TODO potser el delimitador hauria de ser '\' en comptes de ','
                patientOrientationString = this->mapDirectionCosinesToOrientationString( dirCosinesX );
                patientOrientationString += ",";
                patientOrientationString += this->mapDirectionCosinesToOrientationString( dirCosinesY );
                patientOrientationString += ",";
                patientOrientationString += this->mapDirectionCosinesToOrientationString( dirCosinesZ );
                image->setPatientOrientation( patientOrientationString );
            }
        }
        else
        {
            /* Si la modalitat no requereix el image plane module ( CR per exemple ) no disposem de ImageOrientationPatient.
             * Això fa que el tag PatientOrientation no s'ompli.El PatientOrientation es necessari en cas que no hi hagi
             * ImageOrientationPatient i ImagePositionPatient.
             */
            // \TODO Part afegida per sortir del pas. S'hauria de refer aquesta part tenint mes en compte la dependencia de tags. Els
            // replace serveixen perque l'aplicacio funcioni, ja que ara no es preveu que els valors estiguin separts per '\' sino per ','.

            value = dicomReader.getAttributeByName( DCM_PatientOrientation );
            if( !value.isEmpty() )
                image->setPatientOrientation( value.replace( QString("\\") , QString(",") ).replace( QString("F") , QString("I") ).replace( QString("H") , QString("S") ) );
            else
                DEBUG_LOG("No s'ha pogut trobar informació d'orientació del pacient, ni ImageOrientationPatient ni PatientOrientation. Modalitat de la imatge: [" + modality + "]");
        }
        value = dicomReader.getAttributeByName( DCM_ImagePositionPatient );
        if( !value.isEmpty() )
        {
            list = value.split("\\");
            if( list.size() == 3 )
            {
                double position[3] = { list.at(0).toDouble(), list.at(1).toDouble(), list.at(2).toDouble() };
                image->setImagePositionPatient( position );
            }
        }
        else
        {
            DEBUG_LOG("La imatge no conté informació de l'origen. Modalitat: [" + modality + "]");
        }

        image->setSamplesPerPixel( dicomReader.getAttributeByName( DCM_SamplesPerPixel ).toInt() );
        image->setPhotometricInterpretation( dicomReader.getAttributeByName( DCM_PhotometricInterpretation ) );
        image->setRows( dicomReader.getAttributeByName( DCM_Rows ).toInt() );
        image->setColumns( dicomReader.getAttributeByName( DCM_Columns ).toInt() );
        image->setBitsAllocated( dicomReader.getAttributeByName( DCM_BitsAllocated ).toInt() );
        image->setBitsStored( dicomReader.getAttributeByName( DCM_BitsStored ).toInt() );
        image->setPixelRepresentation( dicomReader.getAttributeByName( DCM_PixelRepresentation ).toInt() );

        value = dicomReader.getAttributeByName( DCM_RescaleSlope );
        if( value.toDouble() == 0 )
            image->setRescaleSlope( 1. );
        else
            image->setRescaleSlope( value.toDouble() );

        image->setRescaleIntercept( dicomReader.getAttributeByName( DCM_RescaleIntercept ).toDouble() );
        // llegim els window levels
        QStringList windowWidthList = dicomReader.getAttributeByName( DCM_WindowWidth ).split("\\");
        QStringList windowLevelList = dicomReader.getAttributeByName( DCM_WindowCenter ).split("\\");
        for( int i = 0; i < windowWidthList.size(); i++ )
            image->addWindowLevel( windowWidthList.at(i).toDouble(), windowLevelList.at(i).toDouble() );
        // i després les respectives descripcions si n'hi ha
        image->setWindowLevelExplanations( dicomReader.getAttributeByName( DCM_WindowCenterWidthExplanation ).split("\\") );

        int frames = dicomReader.getAttributeByName( DCM_NumberOfFrames ).toInt();
        image->setNumberOfFrames( frames ? frames : 1 );

        if (dicomReader.tagExists( DCM_KVP ))
        {
            image->setKiloVoltagePeak( dicomReader.getAttributeByName( DCM_KVP ).toDouble() );
        }

        if (dicomReader.tagExists( DCM_ExposureInMicroAs ))
        {
            image->setMicroAmpersSecond( dicomReader.getAttributeByName( DCM_ExposureInMicroAs ).toDouble() );
        }

        if (dicomReader.getSequenceAttributeByName( DCM_CTExposureSequence , DCM_ExposureInmAs ).count() > 0)
        {//Comprovem si tenim la informació dins la seqüència d'exposició, ja el DCM_ExposureInmAs és de tipus 1 si existeix la seqüència Exposure, que conté informació sobre l'exposició del pacient
                image->setMilliAmpersSecond( dicomReader.getSequenceAttributeByName( DCM_CTExposureSequence , DCM_ExposureInmAs )[0].toDouble() );//Accedim a la posició 0 per llegir el valor de MiliAmpers
        }
        else if (dicomReader.tagExists( DCM_Exposure ))
        {//si no existeix al seqüència provem amb el camp DCM_Exposure que conté l'exposició en mAs
            image->setMilliAmpersSecond( dicomReader.getAttributeByName( DCM_Exposure ).toDouble() );
        }

        if (dicomReader.tagExists( DCM_RepetitionTime ))
        {
            image->setRepetitionTime( QString::number( dicomReader.getAttributeByName( DCM_RepetitionTime ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_EchoTime ))
        {
            image->setEchoTime( QString::number( dicomReader.getAttributeByName( DCM_EchoTime ).toDouble() , 'f' , 1 ) );
        }

        if (dicomReader.tagExists( DCM_InversionTime ))
        {
            image->setInversionTime( QString::number( dicomReader.getAttributeByName( DCM_InversionTime ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_SpacingBetweenSlices ))
        {
            image->setSpacingBetweenSlices( QString::number( dicomReader.getAttributeByName( DCM_SpacingBetweenSlices ).toDouble() , 'f' , 1 ) );
        }

        if (dicomReader.tagExists( DCM_VariableFlipAngleFlag ))
        {
            image->setFlipAngle( QString::number( dicomReader.getAttributeByName( DCM_VariableFlipAngleFlag ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_NumberOfAverages ))
        {
            image->setNumberOfAverages( dicomReader.getAttributeByName( DCM_NumberOfAverages ) );
        }

        if (dicomReader.tagExists( DCM_PercentPhaseFieldOfView ))
        {
            image->setPercentPhaseFieldOfView( QString::number( dicomReader.getAttributeByName( DCM_PercentPhaseFieldOfView ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_ReceiveCoilName ))
        {
            image->setReceiveCoilName( dicomReader.getAttributeByName( DCM_ReceiveCoilName ) );
        }

        if (dicomReader.tagExists( DCM_ReconstructionDiameter ))
        {
            image->setReconstructionDiameter( QString::number( dicomReader.getAttributeByName( DCM_ReconstructionDiameter ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_ExposureTime ))
        {
            image->setExposureTime( QString::number( dicomReader.getAttributeByName( DCM_ExposureTime ).toDouble() , 'f' , 2 ) );
        }

        if (dicomReader.tagExists( DCM_TableHeight ))
        {
            image->setTableHeight( QString::number( dicomReader.getAttributeByName( DCM_TableHeight ).toDouble() , 'f' , 0 ) );
        }

        if (dicomReader.tagExists( DCM_SliceLocation ))
        {
            image->setSliceLocation( dicomReader.getAttributeByName( DCM_SliceLocation ) );
        }

        if (dicomReader.tagExists( DCM_FilterType ))
        {
            image->setFilterType( dicomReader.getAttributeByName( DCM_FilterType ) );
        }

        if (dicomReader.tagExists( DCM_ImageType ))
        {
            // aquest valor és de tipus 3 al mòdul General Image, però consta com a tipus 1 a
            // gairebé totes les modalitats. Només consta com a tipus 2 per la modalitat US
            value = dicomReader.getAttributeByName( DCM_ImageType );
            image->setImageType( value );
            if( modality == "CT" ) // en el cas del CT ens interessa saber si és localizer
            {
                QStringList valueList = value.split( "\\" );
                if( valueList.count() >= 3 )
                {
                    if( valueList.at(2) == "LOCALIZER" )
                    {
                        image->setCTLocalizer( true );
                        DEBUG_LOG( " La imatge amb UID " + image->getSOPInstanceUID() + " és un localitzador " );
                    }
                }
                else
                {
                    // TODO aquesta comprovació s'ha afegit perquè hem trobat un cas en que aquestes dades apareixen incoherents
                    // tot i així, lo seu seria disposar d'alguna eina que comprovés si les dades són consistents o no.
                    DEBUG_LOG( "ERROR: Inconsistència DICOM: La imatge " + image->getSOPInstanceUID() + " de la serie " + image->getParentSeries()->getInstanceUID() + " té el camp ImageType que és tipus 1, amb un nombre incorrecte d'elements: Valor del camp:: [" + value + "]" );
                }
            }
        }

        if (dicomReader.tagExists( DCM_ScanArc ))
        {
            image->setScanArc( dicomReader.getAttributeByName( DCM_ScanArc ) );
        }

        if (dicomReader.tagExists( DCM_GantryDetectorTilt ))
        {
            image->setTilt( dicomReader.getAttributeByName( DCM_GantryDetectorTilt ) );
        }

        if (dicomReader.tagExists( DCM_ExposureTime ))
        {
            image->setExposureTime( dicomReader.getAttributeByName( DCM_ExposureTime ) );
        }

        if (dicomReader.tagExists( DCM_FlipAngle))
        {
            image->setFlipAngle( dicomReader.getAttributeByName( DCM_FlipAngle) );
        }
    }
    else
    {
        DEBUG_LOG( "L'arxiu [" + image->getPath() + "] no conté el tag PixelData" );
    }

    return ok;
}

QString ImageFillerStep::mapDirectionCosinesToOrientationString( double vector[3] )
{
    char *orientation = new char[4];
    char *optr = orientation;
    *optr='\0';

    char orientationX = vector[0] < 0 ? 'R' : 'L';
    char orientationY = vector[1] < 0 ? 'A' : 'P';
    char orientationZ = vector[2] < 0 ? 'I' : 'S';

    double absX = fabs( vector[0] );
    double absY = fabs( vector[1] );
    double absZ = fabs( vector[2] );

    int i;
    for ( i = 0; i < 3; ++i )
    {
        if ( absX > .0001 && absX > absY && absX > absZ )
        {
            *optr++= orientationX;
            absX = 0;
        }
        else if ( absY > .0001 && absY > absX && absY > absZ )
        {
            *optr++= orientationY;
            absY = 0;
        }
        else if ( absZ > .0001 && absZ > absX && absZ > absY )
        {
            *optr++= orientationZ;
            absZ = 0;
        }
        else break;
        *optr='\0';
    }
    return QString( orientation );
}

}

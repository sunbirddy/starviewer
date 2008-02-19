/***************************************************************************
 *   Copyright (C) 2005-2006 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#include "patientbrowsermenu.h"

#include <QDesktopWidget>
#include <QApplication>
#include <QVBoxLayout>
#include <QMouseEvent>

#include "patient.h"
#include "patientbrowsermenuextendeditem.h"
#include "patientbrowsermenulist.h"
#include "series.h"
#include "logging.h"

namespace udg {

PatientBrowserMenu::PatientBrowserMenu(QWidget *parent) : QWidget(parent)
{
    m_patientAdditionalInfo = new PatientBrowserMenuExtendedItem(this);
    m_patientBrowserList = new PatientBrowserMenuList(this);

    connect( m_patientAdditionalInfo, SIGNAL( close() ), m_patientBrowserList, SLOT( close() ) );

    m_patientBrowserList->setWindowFlags( Qt::Popup );

    if ( QT_VERSION < 0x040300 ) // Abans de 4.3
    {
        m_patientAdditionalInfo->setWindowFlags( Qt::Popup );
    }
    else
    {
        m_patientAdditionalInfo->setWindowFlags( Qt::SplashScreen );
    }
    
    //TODO Hack per fer desapareixer els 2 popups al clickar fora d'aquest: veure eventFilter
    //S'hauria de fer que el browser i additional info fossin, realment, fills d'aquest widget i tractar-ho com un de sol.
    m_patientAdditionalInfo->installEventFilter(this);
    m_patientBrowserList->installEventFilter(this);
}

PatientBrowserMenu::~PatientBrowserMenu()
{
}

void PatientBrowserMenu::setPatient( Patient * patient )
{
    m_patientBrowserList->setPatient( patient );

    connect(m_patientBrowserList, SIGNAL( isActive(Series*) ), m_patientAdditionalInfo, SLOT( setSeries(Series*) ));
    connect(m_patientBrowserList, SIGNAL( isActive(Series*) ), SLOT( updatePosition() ));
    connect(m_patientBrowserList, SIGNAL( selectedSerie(Series*) ), SLOT ( emitSelected(Series*) ));
}

void PatientBrowserMenu::popup(const QPoint &point, QString serieUID )
{
    // Calcular si el menu hi cap a la pantalla
    int x = point.x();
    int y = point.y();

    int screen_x, screen_y;

    if (qApp->desktop()->isVirtualDesktop())
    {
        screen_x = qApp->desktop()->geometry().width();
        screen_y = qApp->desktop()->geometry().height();
    }
    else
    {
        screen_x = qApp->desktop()->availableGeometry( point ).width();
        screen_y = qApp->desktop()->availableGeometry( point ).height();
    }

    m_patientBrowserList->setSelectedSerie( serieUID );

    QSize widgetIdealSize = m_patientBrowserList->sizeHint();

    if ( ( x + widgetIdealSize.width() ) > screen_x )
    {
        x = screen_x - widgetIdealSize.width() - 5;
    }

    if ( ( y + widgetIdealSize.height() ) > screen_y )
    {
        y = screen_y - widgetIdealSize.height() - 5;
    }

    // moure la finestra del menu al punt que toca
    m_patientBrowserList->move(x, y);
    m_patientBrowserList->show();

    QSize patientAdditionalInfoSize = m_patientAdditionalInfo->sizeHint();

    // Calcular si hi cap a la dreta, altrament el mostrarem a l'esquerre del menu
    if( (m_patientBrowserList->x() + m_patientBrowserList->width() + patientAdditionalInfoSize.width() ) > screen_x )
    {
        x = ( m_patientBrowserList->geometry().x() ) - ( patientAdditionalInfoSize.width() );
    }
    else
    {
        x =  m_patientBrowserList->x() + m_patientBrowserList->width();
    }
    m_patientAdditionalInfo->move( x, m_patientBrowserList->y() );
    m_patientAdditionalInfo->show();

    if ( QT_VERSION >= 0x040300 ) // Amb qt 4.3
    {
        m_patientBrowserList->show();
        //TODO Es fa un updatePosition per tal de moure amb 4.3 el widget adicional al punt que toca, ja que dóna problemes.
        updatePosition();
    }
}

bool PatientBrowserMenu::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QPoint additionalInfoPos = m_patientAdditionalInfo->mapFromGlobal( QCursor::pos() );
        if ( !m_patientAdditionalInfo->rect().contains(additionalInfoPos) )
        {
            m_patientBrowserList->hide();
            m_patientAdditionalInfo->hide();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PatientBrowserMenu::emitSelected( Series * serie )
{
    m_patientBrowserList->hide();
    m_patientAdditionalInfo->hide();
    emit selectedSeries( serie );
}

void PatientBrowserMenu::updatePosition()
{
    int x;
    int screen_x;
    //Passem el point per assegurar-nos que s'agafa la pantalla a on es visualitza el widget
    if (qApp->desktop()->isVirtualDesktop())
    {
        screen_x = qApp->desktop()->geometry().width();
    }
    else
    {
        screen_x = qApp->desktop()->availableGeometry( m_patientBrowserList->pos() ).width();
    }

    QSize patientAdditionalInfoSize = m_patientAdditionalInfo->sizeHint();
    m_patientAdditionalInfo->resize( patientAdditionalInfoSize );

    // Calcular si hi cap a la dreta, altrament el mostrarem a l'esquerre del menu
    if( (m_patientBrowserList->x() + m_patientBrowserList->width() + patientAdditionalInfoSize.width() ) > screen_x )
    {
        x = ( m_patientBrowserList->x() ) -( m_patientAdditionalInfo->frameGeometry().width() );
    }
    else
    {
        x =  m_patientBrowserList->x() + m_patientBrowserList->width();
    }
    m_patientAdditionalInfo->move( x, m_patientBrowserList->y() );
    m_patientAdditionalInfo->show();
}

}

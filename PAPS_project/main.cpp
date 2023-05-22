#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QDebug>

class Note
{
public:
    QString title;
    QString text;
    QDateTime createdAt;

    Note(const QString& title, const QString& text)
        : title(title), text(text), createdAt(QDateTime::currentDateTime())
    {}
};

class NoteListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit NoteListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return notes.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid())
            return QVariant();

        if (index.row() >= notes.size())
            return QVariant();

        if (role == Qt::DisplayRole || role == Qt::EditRole)
        {
            const Note& note = notes.at(index.row());
            return QString("%1: %2").arg(note.title).arg(note.text);
        }

        return QVariant();
    }

    void setNotes(const QList<Note>& noteList)
    {
        beginResetModel();
        notes = noteList;
        endResetModel();
    }

private:
    QList<Note> notes;
};



class NoteView : public QWidget
{
    Q_OBJECT

public:
    explicit NoteView(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        titleLabel = new QLabel(this);
        textLabel = new QLabel(this);
        layout->addWidget(titleLabel);
        layout->addWidget(textLabel);
        setLayout(layout);
    }

    void displayNote(const Note& note)
    {
        titleLabel->setText("Title: " + note.title);
        textLabel->setText("Text: " + note.text + "\nCreated At: " + note.createdAt.toString());
    }

private:
    QLabel* titleLabel;
    QLabel* textLabel;
};

class NoteEditor : public QWidget
{
    Q_OBJECT

public:
    explicit NoteEditor(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        titleEdit = new QLineEdit(this);
        textEdit = new QTextEdit(this);
        saveButton = new QPushButton("Save", this);
        layout->addWidget(titleEdit);
        layout->addWidget(textEdit);
        layout->addWidget(saveButton);
        setLayout(layout);

        connect(saveButton, &QPushButton::clicked, this, &NoteEditor::saveNote);
    }

    void setNoteData(const QString& title, const QString& text)
    {
        titleEdit->setText(title);
        textEdit->setPlainText(text);
    }

signals:
    void noteSaved(const QString& title, const QString& text);

private slots:
    void saveNote()
    {
        QString title = titleEdit->text();
        QString text = textEdit->toPlainText();
        emit noteSaved(title, text);
    }

private:
    QLineEdit* titleEdit;
    QTextEdit* textEdit;
    QPushButton* saveButton;
};

class NoteController : public QObject
{
    Q_OBJECT

public:
    explicit NoteController(NoteListModel* noteListModel, NoteView* noteView, NoteEditor* noteEditor)
        : QObject(), noteListModel(noteListModel), noteView(noteView), noteEditor(noteEditor)
    {
        connect(noteListModel, &NoteListModel::dataChanged, this, &NoteController::onNoteSelectionChanged);
        connect(noteEditor, &NoteEditor::noteSaved, this, &NoteController::saveNote);

        createNoteButton = new QPushButton("Create Note");
        connect(createNoteButton, &QPushButton::clicked, this, &NoteController::createNote);

        deleteNoteButton = new QPushButton("Delete Note");
        connect(deleteNoteButton, &QPushButton::clicked, this, &NoteController::deleteNote);

        QHBoxLayout* layout = new QHBoxLayout();
        layout->addWidget(createNoteButton);
        layout->addWidget(deleteNoteButton);
        layout->addStretch();

        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->addWidget(new QLabel("Note List"));
        noteListView = new QListWidget();
        mainLayout->addWidget(noteListView);
        mainLayout->addLayout(layout);
        mainLayout->addWidget(noteView);
        mainLayout->addWidget(noteEditor);
        noteEditor->hide();
        QWidget* mainWidget = new QWidget();
        mainWidget->setLayout(mainLayout);
        mainWidget->show();

        connect(noteListView, &QListWidget::currentItemChanged, this, &NoteController::onNoteListItemChanged);

        updateNoteList();
    }

private slots:
    void onNoteListItemChanged(QListWidgetItem* currentItem, QListWidgetItem* previousItem)
    {
        if (currentItem)
        {
            int index = noteListView->row(currentItem);
            if (index >= 0 && index < notes.count())
            {
                const Note& note = notes[index];
                noteView->displayNote(note);
            }
        }
        else
        {
            noteView->displayNote(Note("", ""));
        }
    }

    void onNoteSelectionChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
        Q_UNUSED(topLeft);
        Q_UNUSED(bottomRight);

        updateNoteList();
    }

    void createNote()
    {
        noteListView->setCurrentItem(nullptr);
        noteEditor->setNoteData("", "");
        noteEditor->show();
    }

    void deleteNote()
    {
        QListWidgetItem* currentItem = noteListView->currentItem();
        if (currentItem)
        {
            int index = noteListView->row(currentItem);
            if (index >= 0 && index < notes.count())
            {
                notes.removeAt(index);
                updateNoteList();
            }
        }
    }

    void saveNote(const QString& title, const QString& text)
    {
        if (noteListView->currentItem())
        {
            int index = noteListView->row(noteListView->currentItem());
            if (index >= 0 && index < notes.count())
            {
                Note& note = notes[index];
                note.title = title;
                note.text = text;
            }
        }
        else
        {
            Note note(title, text);
            notes.append(note);
        }

        noteEditor->hide();
        updateNoteList();
    }

    void updateNoteList()
    {
        noteListModel->setNotes(notes);
    }

private:
    QList<Note> notes;
    NoteListModel* noteListModel;
    NoteView* noteView;
    NoteEditor* noteEditor;
    QListWidget* noteListView;
    QPushButton* createNoteButton;
    QPushButton* deleteNoteButton;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    NoteListModel noteListModel;
    NoteView noteView;
    NoteEditor noteEditor;

    NoteController noteController(&noteListModel, &noteView, &noteEditor);

    return app.exec();
}

#include "main.moc"

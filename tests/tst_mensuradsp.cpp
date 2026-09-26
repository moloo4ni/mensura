#include "mensuradsp.h"

#include <core/engine/audioformat.h>
#include <core/engine/dsp/processingbuffer.h>
#include <core/engine/dsp/processingbufferlist.h>

#include <QTest>

#include <vector>

using namespace Fooyin;
using namespace Fooyin::Mensura;
using namespace Qt::StringLiterals;

class TestMensuraDsp : public QObject
{
    Q_OBJECT

private slots:
    void identity();
    void passthroughLeavesAudioUntouched();
};

void TestMensuraDsp::identity()
{
    MensuraDsp dsp;
    QCOMPARE(dsp.id(), u"fooyin.dsp.mensura"_s);
    QCOMPARE(dsp.name(), u"Mensura"_s);
    QVERIFY(dsp.loadSettings({}));
    QVERIFY(dsp.saveSettings().isEmpty());
}

void TestMensuraDsp::passthroughLeavesAudioUntouched()
{
    const std::vector<double> input{0.1, -0.2, 0.3, -0.4, 0.5, -0.6};
    MensuraDsp dsp;
    const AudioFormat format{SampleFormat::F64, 48000, 2};
    dsp.prepare(format);

    ProcessingBufferList list;
    list.setToSingle(ProcessingBuffer{input, format, 0});
    dsp.process(list);

    const auto output = list.item(0)->constData();
    QCOMPARE(std::vector<double>(output.begin(), output.end()), input);
}

QTEST_GUILESS_MAIN(TestMensuraDsp)
#include "tst_mensuradsp.moc"

import testtools
from url_dispatcher_testability import fixture_setup, fake_dispatcher

from subprocess import call


class FakeDispatcherTestCase(testtools.TestCase):

    def setUp(self):
        super(FakeDispatcherTestCase, self).setUp()
        self.dispatcher = fixture_setup.FakeURLDispatcher()
        self.useFixture(self.dispatcher)

    def test_url_dispatcher(self):
        call(['url-dispatcher', 'test://testurl'])
        try:
            last_url = self.dispatcher.get_last_dispatch_url_call_parameter()
        except fake_dispatcher.FakeDispatcherException:
            last_url = None
        self.assertEqual(last_url, 'test://testurl')

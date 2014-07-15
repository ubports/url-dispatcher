import testtools
from url_dispatcher_testability import fixture_setup

from subprocess import call

class FakeDispatcherTestCase(testtools.TestCase):

    def setUp(self):
        super(FakeDispatcherTestCase, self).setUp()
        self.dispatcher = fixture_setup.FakeURLDispatcher()
        self.useFixture(self.dispatcher)

    def test_url_dispatcher(self):
        call(['url-dispatcher', 'test://testurl'])
        def get_last_dispatch_url_call_parameter():
            try:
                return self.dispatcher.get_last_dispatch_url_call_parameter()
            except fixture_setup.FakeDispatcherException:
                return None
        
        self.assertEqual(
            get_last_dispatch_url_call_parameter(),
            'test://testurl')
            

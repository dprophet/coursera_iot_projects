from twython import TwythonStreamer


class MyStreamer(TwythonStreamer):
    def on_success(self, data):
        if 'text' in data:
            #print "text data was", data
            self.n_count=self.n_count+1
            print "I have found", self.n_count, self.track_text, "tweets"
            if self.n_count >= 3:
                print self.track_text, "is popular!"


execfile("my_api_keys.py")

stream = MyStreamer(c_k, c_s, a_t, a_s)
stream.n_count = 0
#stream.track_text = 'Trump'
stream.track_text = 'Ian G. Harris'
stream.statuses.filter(track=stream.track_text)
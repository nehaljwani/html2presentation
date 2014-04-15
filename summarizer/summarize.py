#!/usr/bin/env python

import nltk
from nltk.corpus import stopwords
import string
import re

stop_words = stopwords.words('english')

# The low end of shared words to consider
LOWER_BOUND = .20

# The high end, since anything above this is probably SEO garbage or a
# duplicate sentence
UPPER_BOUND = .90


def is_unimportant(word):
    """Decides if a word is ok to toss out for the sentence comparisons"""
    return word in ['.', '!', ',', ] or '\'' in word or word in stop_words


def only_important(sent):
    """Just a little wrapper to filter on is_unimportant"""
    return filter(lambda w: not is_unimportant(w), sent)


def compare_sents(sent1, sent2):
    """Compare two word-tokenized sentences for shared words"""
    if not len(sent1) or not len(sent2):
        return 0
    return len(set(only_important(sent1)) & set(only_important(sent2))) / ((len(sent1) + len(sent2)) / 2.0)


def compare_sents_bounded(sent1, sent2):
    """If the result of compare_sents is not between LOWER_BOUND and
    UPPER_BOUND, it returns 0 instead, so outliers don't mess with the sum"""
    cmpd = compare_sents(sent1, sent2)
    if cmpd <= LOWER_BOUND or cmpd >= UPPER_BOUND:
        return 0
    return cmpd


def compute_score(sent, sents):
    """Computes the average score of sent vs the other sentences (the result of
    sent vs itself isn't counted because it's 1, and that's above
    UPPER_BOUND)"""
    if not len(sent):
        return 0
    return sum(compare_sents_bounded(sent, sent1) for sent1 in sents) / float(len(sents))


def summarize_block(block):
    """Return the sentence that best summarizes block"""
    #print "jkj" 
    if not block:
        return None
    #print "jkj"   
    sents = nltk.sent_tokenize(block)
    word_sents = map(nltk.word_tokenize, sents)
    #print word_sents
    d = dict((compute_score(word_sent, word_sents), sent) for sent, word_sent in zip(sents, word_sents))
    return d;


def find_likely_body(b):
    """Find the tag with the most directly-descended <p> tags"""
    return max(b.find_all(), key=lambda t: len(t.find_all('p', recursive=False)))


class Summary(object):
    def __init__(self, url, article_html, title, summaries):
        self.url = url
        self.article_html = article_html
        self.title = title
        self.summaries = summaries

    def __repr__(self):
        return 'Summary({0}, {1}, {2}, {3})'.format(
                repr(self.url), repr(self.article_html), repr(self.title), repr(self.summaries)
                )

        def __str__(self):
            return u"{0} - {1}\n\n{2}".format(self.title, self.url, '\n'.join(self.summaries))


def summarize_page(text):
    """Summarize the given block and return a max of 4 sentences."""
    sentences = []
    d = summarize_block(text)
    keys = d.keys()
    keys.sort()
    keys.reverse()
    for key in keys:
        sentences.append(d[key])
    s = ""
    if len(sentences) < 4:
        length = len(sentences)
    else:
        lenght = 4
    for i in range(lenght):	
        s = s + " " + sentences[i]	
    return s

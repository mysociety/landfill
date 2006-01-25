# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)


SRC = \
  $(SRCDIR)/attach.c \
  $(SRCDIR)/blob.c \
  $(SRCDIR)/browse.c \
  $(SRCDIR)/cgi.c \
  $(SRCDIR)/common.c \
  $(SRCDIR)/db.c \
  $(SRCDIR)/format.c \
  $(SRCDIR)/history.c \
  $(SRCDIR)/image.c \
  $(SRCDIR)/index.c \
  $(SRCDIR)/login.c \
  $(SRCDIR)/main.c \
  $(SRCDIR)/md5.c \
  $(SRCDIR)/rss.c \
  $(SRCDIR)/search.c \
  $(SRCDIR)/setup.c \
  $(SRCDIR)/test.c \
  $(SRCDIR)/throttle.c \
  $(SRCDIR)/ticket.c \
  $(SRCDIR)/timeline.c \
  $(SRCDIR)/user.c \
  $(SRCDIR)/view.c \
  $(SRCDIR)/wiki.c \
  $(SRCDIR)/wikiinit.c

TRANS_SRC = \
  attach_.c \
  blob_.c \
  browse_.c \
  cgi_.c \
  common_.c \
  db_.c \
  format_.c \
  history_.c \
  image_.c \
  index_.c \
  login_.c \
  main_.c \
  md5_.c \
  rss_.c \
  search_.c \
  setup_.c \
  test_.c \
  throttle_.c \
  ticket_.c \
  timeline_.c \
  user_.c \
  view_.c \
  wiki_.c \
  wikiinit_.c

OBJ = \
  attach.o \
  blob.o \
  browse.o \
  cgi.o \
  common.o \
  db.o \
  format.o \
  history.o \
  image.o \
  index.o \
  login.o \
  main.o \
  md5.o \
  rss.o \
  search.o \
  setup.o \
  test.o \
  throttle.o \
  ticket.o \
  timeline.o \
  user.o \
  view.o \
  wiki.o \
  wikiinit.o

APPNAME = cvstrac$(E)



all:	$(APPNAME) index.html

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

translate:	$(SRCDIR)/translate.c
	$(BCC) -o translate $(SRCDIR)/translate.c

makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o makeheaders $(SRCDIR)/makeheaders.c

mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o mkindex $(SRCDIR)/mkindex.c

makewikiinit:	$(SRCDIR)/makewikiinit.c
	$(BCC) -o makewikiinit $(SRCDIR)/makewikiinit.c $(LIBSQLITE)

$(APPNAME):	headers $(OBJ)
	$(TCC) -o $(APPNAME) $(OBJ) $(LIBSQLITE)

index.html:	$(SRCDIR)/webpage.html $(SRCDIR)/VERSION
	sed -f $(SRCDIR)/VERSION $(SRCDIR)/webpage.html >cvstrac.html

clean:	
	rm -f *.o *_.c $(APPNAME)
	rm -f translate makeheaders mkindex page_index.h index.html headers
	rm -f attach.h blob.h browse.h cgi.h common.h db.h format.h history.h image.h index.h login.h main.h md5.h rss.h search.h setup.h test.h throttle.h ticket.h timeline.h user.h view.h wiki.h wikiinit.h

headers:	makeheaders mkindex $(TRANS_SRC)
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	./mkindex $(TRANS_SRC) >page_index.h
	touch headers

attach_.c:	$(SRCDIR)/attach.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/attach.c | sed -f $(SRCDIR)/VERSION >attach_.c

attach.o:	attach_.c attach.h 
	$(XTCC) -o attach.o -c attach_.c

attach.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

blob_.c:	$(SRCDIR)/blob.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/blob.c | sed -f $(SRCDIR)/VERSION >blob_.c

blob.o:	blob_.c blob.h 
	$(XTCC) -o blob.o -c blob_.c

blob.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

browse_.c:	$(SRCDIR)/browse.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/browse.c | sed -f $(SRCDIR)/VERSION >browse_.c

browse.o:	browse_.c browse.h 
	$(XTCC) -o browse.o -c browse_.c

browse.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

cgi_.c:	$(SRCDIR)/cgi.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/cgi.c | sed -f $(SRCDIR)/VERSION >cgi_.c

cgi.o:	cgi_.c cgi.h 
	$(XTCC) -o cgi.o -c cgi_.c

cgi.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

common_.c:	$(SRCDIR)/common.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/common.c | sed -f $(SRCDIR)/VERSION >common_.c

common.o:	common_.c common.h 
	$(XTCC) -o common.o -c common_.c

common.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

db_.c:	$(SRCDIR)/db.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/db.c | sed -f $(SRCDIR)/VERSION >db_.c

db.o:	db_.c db.h 
	$(XTCC) -o db.o -c db_.c

db.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

format_.c:	$(SRCDIR)/format.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/format.c | sed -f $(SRCDIR)/VERSION >format_.c

format.o:	format_.c format.h 
	$(XTCC) -o format.o -c format_.c

format.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

history_.c:	$(SRCDIR)/history.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/history.c | sed -f $(SRCDIR)/VERSION >history_.c

history.o:	history_.c history.h 
	$(XTCC) -o history.o -c history_.c

history.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

image_.c:	$(SRCDIR)/image.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/image.c | sed -f $(SRCDIR)/VERSION >image_.c

image.o:	image_.c image.h 
	$(XTCC) -o image.o -c image_.c

image.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

index_.c:	$(SRCDIR)/index.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/index.c | sed -f $(SRCDIR)/VERSION >index_.c

index.o:	index_.c index.h 
	$(XTCC) -o index.o -c index_.c

index.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

login_.c:	$(SRCDIR)/login.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/login.c | sed -f $(SRCDIR)/VERSION >login_.c

login.o:	login_.c login.h 
	$(XTCC) -o login.o -c login_.c

login.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

main_.c:	$(SRCDIR)/main.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/main.c | sed -f $(SRCDIR)/VERSION >main_.c

main.o:	main_.c main.h page_index.h
	$(XTCC) -o main.o -c main_.c

main.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

md5_.c:	$(SRCDIR)/md5.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/md5.c | sed -f $(SRCDIR)/VERSION >md5_.c

md5.o:	md5_.c md5.h 
	$(XTCC) -o md5.o -c md5_.c

md5.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

rss_.c:	$(SRCDIR)/rss.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/rss.c | sed -f $(SRCDIR)/VERSION >rss_.c

rss.o:	rss_.c rss.h 
	$(XTCC) -o rss.o -c rss_.c

rss.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

search_.c:	$(SRCDIR)/search.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/search.c | sed -f $(SRCDIR)/VERSION >search_.c

search.o:	search_.c search.h 
	$(XTCC) -o search.o -c search_.c

search.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

setup_.c:	$(SRCDIR)/setup.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/setup.c | sed -f $(SRCDIR)/VERSION >setup_.c

setup.o:	setup_.c setup.h 
	$(XTCC) -o setup.o -c setup_.c

setup.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

test_.c:	$(SRCDIR)/test.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/test.c | sed -f $(SRCDIR)/VERSION >test_.c

test.o:	test_.c test.h 
	$(XTCC) -o test.o -c test_.c

test.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

throttle_.c:	$(SRCDIR)/throttle.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/throttle.c | sed -f $(SRCDIR)/VERSION >throttle_.c

throttle.o:	throttle_.c throttle.h 
	$(XTCC) -o throttle.o -c throttle_.c

throttle.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

ticket_.c:	$(SRCDIR)/ticket.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/ticket.c | sed -f $(SRCDIR)/VERSION >ticket_.c

ticket.o:	ticket_.c ticket.h 
	$(XTCC) -o ticket.o -c ticket_.c

ticket.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

timeline_.c:	$(SRCDIR)/timeline.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/timeline.c | sed -f $(SRCDIR)/VERSION >timeline_.c

timeline.o:	timeline_.c timeline.h 
	$(XTCC) -o timeline.o -c timeline_.c

timeline.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

user_.c:	$(SRCDIR)/user.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/user.c | sed -f $(SRCDIR)/VERSION >user_.c

user.o:	user_.c user.h 
	$(XTCC) -o user.o -c user_.c

user.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

view_.c:	$(SRCDIR)/view.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/view.c | sed -f $(SRCDIR)/VERSION >view_.c

view.o:	view_.c view.h 
	$(XTCC) -o view.o -c view_.c

view.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

wiki_.c:	$(SRCDIR)/wiki.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/wiki.c | sed -f $(SRCDIR)/VERSION >wiki_.c

wiki.o:	wiki_.c wiki.h 
	$(XTCC) -o wiki.o -c wiki_.c

wiki.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers

wikiinit_.c:	$(SRCDIR)/wikiinit.c $(SRCDIR)/VERSION translate
	./translate $(SRCDIR)/wikiinit.c | sed -f $(SRCDIR)/VERSION >wikiinit_.c

wikiinit.o:	wikiinit_.c wikiinit.h 
	$(XTCC) -o wikiinit.o -c wikiinit_.c

wikiinit.h:	makeheaders
	./makeheaders  attach_.c:attach.h blob_.c:blob.h browse_.c:browse.h cgi_.c:cgi.h common_.c:common.h db_.c:db.h format_.c:format.h history_.c:history.h image_.c:image.h index_.c:index.h login_.c:login.h main_.c:main.h md5_.c:md5.h rss_.c:rss.h search_.c:search.h setup_.c:setup.h test_.c:test.h throttle_.c:throttle.h ticket_.c:ticket.h timeline_.c:timeline.h user_.c:user.h view_.c:view.h wiki_.c:wiki.h wikiinit_.c:wikiinit.h
	touch headers


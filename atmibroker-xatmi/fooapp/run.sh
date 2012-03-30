# ALLOW JOBS TO BE BACKGROUNDED
set -m

echo "Quickstart: Running fooapp"

cd $BLACKTIE_HOME/quickstarts/xatmi/fooapp

# RUN THE FOOAPP SERVER
generate_server -Dservice.names=FOOAPP -Dserver.includes=BarService.c -Dserver.name=fooapp
if [ "$?" != "0" ]; then
	exit -1
fi
export BLACKTIE_CONFIGURATION=linux
btadmin startup
if [ "$?" != "0" ]; then
	exit -1
fi
unset BLACKTIE_CONFIGURATION

# RUN THE C CLIENT
generate_client -Dclient.includes=client.c
./client
if [ "$?" != "0" ]; then
	killall -9 server
	exit -1
fi

btadmin shutdown
if [ "$?" != "0" ]; then
	exit -1
fi

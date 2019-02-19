#!/bin/bash


#verify we have the correct nb of arguments
if [ $# -ne 13 ]
then
	echo ""
	echo "$# arguments, required 13"
	echo "usage $0 nb_nodes sim_length bop_algo bop_slots sf_algo nb_parents depth-metric bo so inter_pk_time_unicast(ms) inter_pk_time_multicast(ms) multicast_dest multicast_algo"
	exit
fi



#PARAMETERS RENAMING
NB_NODES=$1
SIM_LENGTH=$2
BOP_ALGO=$3
BOP_SLOTS=$4
SF_ALGO=$5
NB_PARENTS=$6
DEPTH_METRIC=$7
BO=$8
SO=$9
UNICAST_PKTIME=${10}
MULTICAST_PKTIME=${11}
MULTICAST_DEST=${12}
MULTICAST_ALGO=${13}



#DEFAULT VALUES
SCAN_BOPERIOD=10
DURATION=900


echo "<?xml version='1.0' encoding='UTF-8'?>"
echo "<worldsens xmlns=\"http://worldsens.citi.insa-lyon.fr\">"
echo ""

echo "<!-- == Worldsens ===================================================== -->"
echo "<simulation nodes=\"$NB_NODES\" duration=\"`echo $DURATION`s\" x=\"$SIM_LENGTH\" y=\"$SIM_LENGTH\" z=\"0\"/>"
echo ""

echo "<!-- == Entities ====================================================== -->"
echo ""

echo "<!-- == PROPAGATION, INTERFERENCES and MODULATION ===================== -->"


echo "<entity name=\"propagation\" library=\"propagation_shadowing_802154\" >"
echo "<init frequency_MHz=\"868\" pathloss=\"1.97\" deviation=\"2.1\" dist0=\"2.0\" Pr_dBm0=\"-61.4\"/>"
echo "</entity>"
echo ""

echo "<entity name=\"interference\" library=\"interferences_orthogonal\">"
echo "</entity>"
echo ""

echo "<entity name=\"modulation\" library=\"modulation_bpsk\">"
echo "</entity>"
echo ""



echo "<!-- == ANTENNA ============================================ -->"
echo ""

echo "<entity name=\"omnidirectionnal\" library=\"antenna_omnidirectionnal\" >"
echo "</entity>"
echo ""


echo "<!-- == RADIO ============================================ -->"
echo ""
echo "<entity name=\"radio\" library=\"radio_generic\" >"
echo "  <default sensibility=\"-95dBm\" T_s=\"4000\" channel=\"0\" modulation=\"modulation\"/>"
echo "</entity>"
echo ""


echo "<!-- == MAC ===================================================== -->"
echo "<entity name=\"mac\" library=\"mac_802154_slotted\" >"
echo "  <default BO=\"$BO\" SO=\"$SO\" bop-slots=\"$BOP_SLOTS\" bop-algo=\"$BOP_ALGO\" sf-algo=\"$SF_ALGO\" nbmax-parents=\"$NB_PARENTS\" depth-metric=\"$DEPTH_METRIC\" multicast-algo=\"$MULTICAST_ALGO\" hello-boperiod=\"1\" monitorbeacons=\"1\" scan-boperiod=\"$SCAN_BOPERIOD\"  debug=\"0\"/>"
echo "</entity>"

echo ""


echo "<!-- == APPLI ===================================================== -->"
echo "<entity name=\"cbr\" library=\"application_cbr_v3\">"
echo "    <default destination=\"0\" start=\"90s\" unicast-period=\"`echo $UNICAST_PKTIME`ms\" multicast-period=\"`echo $MULTICAST_PKTIME`ms\" multicast-dest=\"$MULTICAST_DEST\"/> "
echo "</entity>"
echo ""


echo "<!-- == MOBILITY ===================================================== -->"
echo "<entity name=\"mobility\" library=\"mobility_static_disk\">"
echo "</entity>"
echo ""


echo "<!-- == Environment ===================================================== -->"
echo "<environment>"
echo "<propagation entity=\"propagation\"/>"
echo "<interferences entity=\"interference\"/>"
echo "<modulation entity=\"modulation\"/>"
echo "</environment>"



echo "<!-- == Bundle ===================================================== -->"
echo "<bundle name=\"node\" worldsens=\"false\" default=\"true\" birth=\"0\">"
echo "  <mobility entity=\"mobility\"/>"
echo ""

echo "  <antenna entity=\"omnidirectionnal\">"
echo "    <up entity=\"radio\"/>"
echo "  </antenna>"
echo ""

echo "  <with entity=\"radio\">"
echo "    <up entity=\"mac\"/>"
echo "    <down entity=\"omnidirectionnal\"/>"
echo "  </with>"
echo ""

echo "  <with entity=\"mac\">"
echo "    <up entity=\"cbr\"/>"
echo "    <down entity=\"radio\"/>"
echo "  </with>"
echo ""


echo "  <with entity=\"cbr\">"
echo "	<down entity=\"mac\"/>"
echo "  </with>"
echo ""

echo "</bundle>"
echo ""

echo "<!-- == Nodes ===================================================== -->"
echo ""

echo "</worldsens>"
echo ""



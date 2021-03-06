# given cndb_log on stdin, generate c code to write raw OMF on stdout
BEGIN {
	i = 0
	arr[i++] = "ver"
	arr[i++] = "info"
	arr[i++] = "tx"
	arr[i++] = "txc"
	arr[i++] = "txm"
	# arr[i++] = "txd"
	arr[i++] = "ack"
	# arr[i++] = "nak"
	printf("/* this file is auto-generated by cndblog2c.awk, see Readme.txt */\n")
	printf("void\ninject_raw(\n\tstruct mpool_mdc *mdc)\n{\n");
	for (i in arr) {
		printf("\tstruct cndb_%s_omf %s = { };\n", arr[i], arr[i])
	}
	printf("\tstruct cndb_oid_omf *oidv = NULL;\n")
	printf("\tchar cndb_buf[CNDB_CBUFSZ_DEFAULT] = { };\n")
	printf("\tmerr_t err = 0;\n")
	printf("\tint i;\n")
	print ""
}

function set_hdr(type, var, extra) {
	printf("\tomf_set_cnhdr_type(&%s.hdr, %s);\n", var, type)
	printf("\tomf_set_cnhdr_len(&%s.hdr, (sizeof(%s) + %s) - sizeof(%s.hdr));\n", var, var, extra, var)
}

$2 == "ver" {
	set_hdr("CNDB_TYPE_VERSION", "ver", "0")
	printf("\tomf_set_cnver_magic(&ver, %s);\n", $4)
	printf("\tomf_set_cnver_version(&ver, %s);\n", $6)
	printf("\tomf_set_cnver_captgt(&ver, %s);\n", $8)
	printf("\terr = mpool_mdc_append(mdc, &ver, sizeof(ver), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "info" {
	msz = $12
	printf("\tomf_set_cninfo_cnid(&info, %s);\n", $4)
	printf("\tomf_set_cninfo_fanout_bits(&info, %s);\n", $6)
	printf("\tomf_set_cninfo_prefix_len(&info, %s);\n", $8)
	printf("\tomf_set_cninfo_flags(&info, %s);\n", $10)
	printf("\tomf_set_cninfo_metasz(&info, %s);\n", $12)
	printf("\tomf_set_cninfo_name(&info, \"%s\", strlen(\"%s\"));\n", $14, $14)
	printf("\tmemset(cndb_buf, 0, sizeof(cndb_buf));\n")
	printf("\ti = sizeof(info);\n")
	for (i = 16; i <= NF; i++) {
		printf("\tcndb_buf[i++] = %s;\n", $i)
	}
	set_hdr("CNDB_TYPE_INFO", "info", msz)
	printf("\tmemcpy(cndb_buf, &info, sizeof(info));\n")
	printf("\terr = mpool_mdc_append(mdc, cndb_buf, sizeof(info) + %u, false);\n",msz)
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "infod" {
	set_hdr("CNDB_TYPE_INFOD", "info", "0")
	printf("\tomf_set_cninfo_cnid(&info, %s);\n", $4)
	printf("\tomf_set_cninfo_fanout_bits(&info, %s);\n", $6)
	printf("\tomf_set_cninfo_prefix_len(&info, %s);\n", $8)
	printf("\tomf_set_cninfo_flags(&info, %s);\n", $10)
	printf("\tomf_set_cninfo_name(&info, \"%s\", strlen(\"%s\"));\n", $12, $12)
	printf("\terr = mpool_mdc_append(mdc, &info, sizeof(info), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "tx" {
	set_hdr("CNDB_TYPE_TX", "tx", "0")
	printf("\tomf_set_tx_id(&tx, %s);\n", $3)
	printf("\tomf_set_tx_seqno(&tx, %s);\n", $5)
	printf("\tomf_set_tx_nc(&tx, %s);\n", $7)
	printf("\tomf_set_tx_nd(&tx, %s);\n", $9)
	printf("\tomf_set_tx_ingestid(&tx, %s);\n", $11)
	printf("\terr = mpool_mdc_append(mdc, &tx, sizeof(tx), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "txc" {
	printf("\tomf_set_txc_id(&txc, %s);\n", $3)
	printf("\tomf_set_txc_tag(&txc, %s);\n", $5)
	printf("\tomf_set_txc_cnid(&txc, %s);\n", $7)
	printf("\tomf_set_txc_keepvbc(&txc, %s);\n", $9)
	printf("\tomf_set_txc_kcnt(&txc, %s);\n", $11)
	printf("\tomf_set_txc_vcnt(&txc, %s);\n", $13)
	printf("\tomf_set_txc_mcnt(&txc, %s);\n", $15)
	printf("\tmemset(cndb_buf, 0, sizeof(cndb_buf));\n")
	printf("\toidv = (void *)&cndb_buf[sizeof(txc)];\n")
	for (i = 17; i <= NF; i++) {
		if ($i != "/")
			printf("\tomf_set_cndb_oid(oidv++, %s);\n", $i)
	}
	set_hdr("CNDB_TYPE_TXC", "txc", "(char *)oidv - &cndb_buf[sizeof(txc)]")
	printf("\tmemcpy(cndb_buf, &txc, sizeof(txc));\n")
	printf("\terr = mpool_mdc_append(mdc, cndb_buf, (char *)oidv - cndb_buf, false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "txm" {
	set_hdr("CNDB_TYPE_TXM", "txm", "0")
	printf("\tomf_set_txm_id(&txm, %s);\n", $3)
	printf("\tomf_set_txm_tag(&txm, %s);\n", $5)
	printf("\tomf_set_txm_cnid(&txm, %s);\n", $7)
	printf("\tomf_set_txm_dgen(&txm, %s);\n", $9)
	split($11, loc, ",")
	printf("\tomf_set_txm_level(&txm, %s);\n", loc[1])
	printf("\tomf_set_txm_offset(&txm, %s);\n", loc[2])
	printf("\tomf_set_txm_vused(&txm, %s);\n", $13)
	printf("\tomf_set_txm_compc(&txm, %s);\n", $15)

	printf("\terr = mpool_mdc_append(mdc, &txm, sizeof(txm), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "txd" {
	printf("\tomf_set_txd_id(&txd, %s);\n", $3)
	printf("\tomf_set_txd_tag(&txd, %s);\n", $5)
	printf("\tomf_set_txd_cnid(&txd, %s);\n", $7)
	printf("\tomf_set_txd_n_oids(&txd, %s);\n", $9)

	printf("\tmemset(cndb_buf, 0, sizeof(cndb_buf));\n")
	printf("\toidv = (void *)&cndb_buf[sizeof(txd)];\n")
	for (i = 11; i <= NF; i++) {
		printf("\tomf_set_cndb_oid(oidv++, %s);\n", $i)
	}
	set_hdr("CNDB_TYPE_TXD", "txd", "(char *)oidv - &cndb_buf[sizeof(txd)]")
	printf("\tmemcpy(cndb_buf, &txd, sizeof(txd));\n")
	printf("\terr = mpool_mdc_append(mdc, cndb_buf, (char *)oidv - cndb_buf, false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "ack-C" || $2 == "ack-D" {
	set_hdr("CNDB_TYPE_ACK", "ack", "0")
	printf("\tomf_set_ack_txid(&ack, %s);\n", $3)
	printf("\tomf_set_ack_tag(&ack, %s);\n", $5)
	printf("\tomf_set_ack_cnid(&ack, %s);\n", $7)
	if ($2 == "ack-C")
		printf("\tomf_set_ack_type(&ack, CNDB_ACK_TYPE_C);\n")
	else
		printf("\tomf_set_ack_type(&ack, CNDB_ACK_TYPE_D);\n")
	printf("\terr = mpool_mdc_append(mdc, &ack, sizeof(ack), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

$2 == "nak" {
	set_hdr("CNDB_TYPE_NAK", "nak", "0")
	printf("\tomf_set_nak_txid(&nak, %s);\n", $3)
	printf("\terr = mpool_mdc_append(mdc, &nak, sizeof(nak), false);\n")
	printf("\tif (ev(err))\n\t\tgoto errout;\n")
	print ""
}

END {
	printf("\treturn;\n\nerrout:\n\tfatal((char *)__func__, err);\n}\n\n")
}

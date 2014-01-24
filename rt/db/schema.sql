-- This dump is just for easy reference, it is not the source schema.
-- Made with "pg_dump --schema-only rt" on bitter, 2006-08-03

--
-- PostgreSQL database dump
--

SET client_encoding = 'UNICODE';
SET check_function_bodies = false;

SET SESSION AUTHORIZATION 'postgres';

SET search_path = public, pg_catalog;

--
-- TOC entry 165 (OID 17142)
-- Name: plpgsql_call_handler(); Type: FUNC PROCEDURAL LANGUAGE; Schema: public; Owner: postgres
--

CREATE FUNCTION plpgsql_call_handler() RETURNS language_handler
    AS '$libdir/plpgsql', 'plpgsql_call_handler'
    LANGUAGE c;


SET SESSION AUTHORIZATION DEFAULT;

--
-- TOC entry 164 (OID 17143)
-- Name: plpgsql; Type: PROCEDURAL LANGUAGE; Schema: public; Owner: 
--

CREATE TRUSTED PROCEDURAL LANGUAGE plpgsql HANDLER plpgsql_call_handler;


SET SESSION AUTHORIZATION 'postgres';

--
-- TOC entry 4 (OID 2200)
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
GRANT ALL ON SCHEMA public TO PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 5 (OID 13865220)
-- Name: attachments_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE attachments_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 6 (OID 13865220)
-- Name: attachments_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE attachments_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 45 (OID 13865222)
-- Name: attachments; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE attachments (
    id integer DEFAULT nextval('attachments_id_seq'::text) NOT NULL,
    transactionid integer NOT NULL,
    parent integer DEFAULT 0 NOT NULL,
    messageid character varying(160),
    subject character varying(255),
    filename character varying(255),
    contenttype character varying(80),
    contentencoding character varying(80),
    content text,
    headers text,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone
);


--
-- TOC entry 46 (OID 13865222)
-- Name: attachments; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE attachments FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 7 (OID 13865235)
-- Name: queues_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE queues_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 8 (OID 13865235)
-- Name: queues_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE queues_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 47 (OID 13865237)
-- Name: queues; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE queues (
    id integer DEFAULT nextval('queues_id_seq'::text) NOT NULL,
    name character varying(200) NOT NULL,
    description character varying(255),
    correspondaddress character varying(120),
    commentaddress character varying(120),
    initialpriority integer DEFAULT 0 NOT NULL,
    finalpriority integer DEFAULT 0 NOT NULL,
    defaultduein integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 48 (OID 13865237)
-- Name: queues; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE queues FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 9 (OID 13865252)
-- Name: links_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE links_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 10 (OID 13865252)
-- Name: links_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE links_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 49 (OID 13865254)
-- Name: links; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE links (
    id integer DEFAULT nextval('links_id_seq'::text) NOT NULL,
    base character varying(240),
    target character varying(240),
    "type" character varying(20) NOT NULL,
    localtarget integer DEFAULT 0 NOT NULL,
    localbase integer DEFAULT 0 NOT NULL,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone
);


--
-- TOC entry 50 (OID 13865254)
-- Name: links; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE links FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 11 (OID 13865265)
-- Name: principals_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE principals_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 12 (OID 13865265)
-- Name: principals_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE principals_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 51 (OID 13865267)
-- Name: principals; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE principals (
    id integer DEFAULT nextval('principals_id_seq'::text) NOT NULL,
    principaltype character varying(16) NOT NULL,
    objectid integer,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 52 (OID 13865267)
-- Name: principals; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE principals FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 13 (OID 13865274)
-- Name: groups_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE groups_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 14 (OID 13865274)
-- Name: groups_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE groups_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 53 (OID 13865276)
-- Name: groups; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE groups (
    id integer DEFAULT nextval('groups_id_seq'::text) NOT NULL,
    name character varying(200),
    description character varying(255),
    "domain" character varying(64),
    "type" character varying(64),
    instance integer
);


--
-- TOC entry 54 (OID 13865276)
-- Name: groups; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE groups FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 15 (OID 13865283)
-- Name: scripconditions_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE scripconditions_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 16 (OID 13865283)
-- Name: scripconditions_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scripconditions_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 55 (OID 13865285)
-- Name: scripconditions; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE scripconditions (
    id integer DEFAULT nextval('scripconditions_id_seq'::text) NOT NULL,
    name character varying(200),
    description character varying(255),
    execmodule character varying(60),
    argument character varying(255),
    applicabletranstypes character varying(60),
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 56 (OID 13865285)
-- Name: scripconditions; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scripconditions FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 17 (OID 13865295)
-- Name: transactions_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE transactions_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 18 (OID 13865295)
-- Name: transactions_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE transactions_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 57 (OID 13865297)
-- Name: transactions; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE transactions (
    id integer DEFAULT nextval('transactions_id_seq'::text) NOT NULL,
    objecttype character varying(255) NOT NULL,
    objectid integer DEFAULT 0 NOT NULL,
    timetaken integer DEFAULT 0 NOT NULL,
    "type" character varying(20),
    field character varying(40),
    oldvalue character varying(255),
    newvalue character varying(255),
    referencetype character varying(255),
    oldreference integer,
    newreference integer,
    data character varying(255),
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone
);


--
-- TOC entry 58 (OID 13865297)
-- Name: transactions; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE transactions FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 19 (OID 13865309)
-- Name: scrips_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE scrips_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 20 (OID 13865309)
-- Name: scrips_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scrips_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 59 (OID 13865311)
-- Name: scrips; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE scrips (
    id integer DEFAULT nextval('scrips_id_seq'::text) NOT NULL,
    description character varying(255),
    scripcondition integer DEFAULT 0 NOT NULL,
    scripaction integer DEFAULT 0 NOT NULL,
    conditionrules text,
    actionrules text,
    customisapplicablecode text,
    custompreparecode text,
    customcommitcode text,
    stage character varying(32),
    queue integer DEFAULT 0 NOT NULL,
    "template" integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 60 (OID 13865311)
-- Name: scrips; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scrips FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 21 (OID 13865325)
-- Name: acl_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE acl_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 22 (OID 13865325)
-- Name: acl_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE acl_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 61 (OID 13865327)
-- Name: acl; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE acl (
    id integer DEFAULT nextval('acl_id_seq'::text) NOT NULL,
    principaltype character varying(25) NOT NULL,
    principalid integer NOT NULL,
    rightname character varying(25) NOT NULL,
    objecttype character varying(25) NOT NULL,
    objectid integer DEFAULT 0 NOT NULL,
    delegatedby integer DEFAULT 0 NOT NULL,
    delegatedfrom integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 62 (OID 13865327)
-- Name: acl; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE acl FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 23 (OID 13865336)
-- Name: groupmembers_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE groupmembers_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 24 (OID 13865336)
-- Name: groupmembers_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE groupmembers_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 63 (OID 13865338)
-- Name: groupmembers; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE groupmembers (
    id integer DEFAULT nextval('groupmembers_id_seq'::text) NOT NULL,
    groupid integer DEFAULT 0 NOT NULL,
    memberid integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 64 (OID 13865338)
-- Name: groupmembers; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE groupmembers FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 25 (OID 13865345)
-- Name: cachedgroupmembers_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE cachedgroupmembers_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 26 (OID 13865345)
-- Name: cachedgroupmembers_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE cachedgroupmembers_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 65 (OID 13865347)
-- Name: cachedgroupmembers; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE cachedgroupmembers (
    id integer DEFAULT nextval('cachedgroupmembers_id_seq'::text) NOT NULL,
    groupid integer,
    memberid integer,
    via integer,
    immediateparentid integer,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 66 (OID 13865347)
-- Name: cachedgroupmembers; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE cachedgroupmembers FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 27 (OID 13865356)
-- Name: users_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE users_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 28 (OID 13865356)
-- Name: users_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE users_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 67 (OID 13865358)
-- Name: users; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE users (
    id integer DEFAULT nextval('users_id_seq'::text) NOT NULL,
    name character varying(200) NOT NULL,
    "password" character varying(40),
    comments text,
    signature text,
    emailaddress character varying(120),
    freeformcontactinfo text,
    organization character varying(200),
    realname character varying(120),
    nickname character varying(16),
    lang character varying(16),
    emailencoding character varying(16),
    webencoding character varying(16),
    externalcontactinfoid character varying(100),
    contactinfosystem character varying(30),
    externalauthid character varying(100),
    authsystem character varying(30),
    gecos character varying(16),
    homephone character varying(30),
    workphone character varying(30),
    mobilephone character varying(30),
    pagerphone character varying(30),
    address1 character varying(200),
    address2 character varying(200),
    city character varying(100),
    state character varying(100),
    zip character varying(16),
    country character varying(50),
    timezone character varying(50),
    pgpkey text,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 68 (OID 13865358)
-- Name: users; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE users FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 29 (OID 13865372)
-- Name: tickets_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE tickets_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 30 (OID 13865372)
-- Name: tickets_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE tickets_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 69 (OID 13865374)
-- Name: tickets; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE tickets (
    id integer DEFAULT nextval('tickets_id_seq'::text) NOT NULL,
    effectiveid integer DEFAULT 0 NOT NULL,
    queue integer DEFAULT 0 NOT NULL,
    "type" character varying(16),
    issuestatement integer DEFAULT 0 NOT NULL,
    resolution integer DEFAULT 0 NOT NULL,
    "owner" integer DEFAULT 0 NOT NULL,
    subject character varying(200) DEFAULT '[no subject]'::character varying,
    initialpriority integer DEFAULT 0 NOT NULL,
    finalpriority integer DEFAULT 0 NOT NULL,
    priority integer DEFAULT 0 NOT NULL,
    timeestimated integer DEFAULT 0 NOT NULL,
    timeworked integer DEFAULT 0 NOT NULL,
    status character varying(10),
    timeleft integer DEFAULT 0 NOT NULL,
    told timestamp without time zone,
    starts timestamp without time zone,
    started timestamp without time zone,
    due timestamp without time zone,
    resolved timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 70 (OID 13865374)
-- Name: tickets; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE tickets FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 31 (OID 13865399)
-- Name: scripactions_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE scripactions_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 32 (OID 13865399)
-- Name: scripactions_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scripactions_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 71 (OID 13865401)
-- Name: scripactions; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE scripactions (
    id integer DEFAULT nextval('scripactions_id_seq'::text) NOT NULL,
    name character varying(200),
    description character varying(255),
    execmodule character varying(60),
    argument character varying(255),
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 72 (OID 13865401)
-- Name: scripactions; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE scripactions FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 33 (OID 13865411)
-- Name: templates_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE templates_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 34 (OID 13865411)
-- Name: templates_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE templates_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 73 (OID 13865413)
-- Name: templates; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE templates (
    id integer DEFAULT nextval('templates_id_seq'::text) NOT NULL,
    queue integer DEFAULT 0 NOT NULL,
    name character varying(200) NOT NULL,
    description character varying(255),
    "type" character varying(16),
    "language" character varying(16),
    translationof integer DEFAULT 0 NOT NULL,
    content text,
    lastupdated timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone
);


--
-- TOC entry 74 (OID 13865413)
-- Name: templates; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE templates FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 35 (OID 13865425)
-- Name: objectcustomfieldvalues_id_s; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE objectcustomfieldvalues_id_s
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 36 (OID 13865425)
-- Name: objectcustomfieldvalues_id_s; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE objectcustomfieldvalues_id_s FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 75 (OID 13865427)
-- Name: objectcustomfieldvalues; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE objectcustomfieldvalues (
    id integer DEFAULT nextval('objectcustomfieldvalues_id_s'::text) NOT NULL,
    customfield integer NOT NULL,
    objecttype character varying(255),
    objectid integer NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    content character varying(255),
    largecontent text,
    contenttype character varying(80),
    contentencoding character varying(80),
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 76 (OID 13865427)
-- Name: objectcustomfieldvalues; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE objectcustomfieldvalues FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 37 (OID 13865441)
-- Name: customfields_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE customfields_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 38 (OID 13865441)
-- Name: customfields_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE customfields_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 77 (OID 13865443)
-- Name: customfields; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE customfields (
    id integer DEFAULT nextval('customfields_id_seq'::text) NOT NULL,
    name character varying(200),
    "type" character varying(200),
    maxvalues integer DEFAULT 0 NOT NULL,
    repeated integer DEFAULT 0 NOT NULL,
    pattern character varying(255),
    lookuptype character varying(255) NOT NULL,
    description character varying(255),
    sortorder integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone,
    disabled integer DEFAULT 0 NOT NULL
);


--
-- TOC entry 78 (OID 13865443)
-- Name: customfields; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE customfields FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 39 (OID 13865457)
-- Name: objectcustomfields_id_s; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE objectcustomfields_id_s
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 40 (OID 13865457)
-- Name: objectcustomfields_id_s; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE objectcustomfields_id_s FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 79 (OID 13865459)
-- Name: objectcustomfields; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE objectcustomfields (
    id integer DEFAULT nextval('objectcustomfields_id_s'::text) NOT NULL,
    customfield integer NOT NULL,
    objectid integer NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 80 (OID 13865459)
-- Name: objectcustomfields; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE objectcustomfields FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 41 (OID 13865467)
-- Name: customfieldvalues_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE customfieldvalues_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 42 (OID 13865467)
-- Name: customfieldvalues_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE customfieldvalues_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 81 (OID 13865469)
-- Name: customfieldvalues; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE customfieldvalues (
    id integer DEFAULT nextval('customfieldvalues_id_seq'::text) NOT NULL,
    customfield integer NOT NULL,
    name character varying(200),
    description character varying(255),
    sortorder integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 82 (OID 13865469)
-- Name: customfieldvalues; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE customfieldvalues FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 43 (OID 13865478)
-- Name: attributes_id_seq; Type: SEQUENCE; Schema: public; Owner: rt
--

CREATE SEQUENCE attributes_id_seq
    INCREMENT BY 1
    NO MAXVALUE
    NO MINVALUE
    CACHE 1;


--
-- TOC entry 44 (OID 13865478)
-- Name: attributes_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE attributes_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 83 (OID 13865480)
-- Name: attributes; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE attributes (
    id integer DEFAULT nextval('attributes_id_seq'::text) NOT NULL,
    name character varying(255) NOT NULL,
    description character varying(255),
    content text,
    contenttype character varying(16),
    objecttype character varying(64),
    objectid integer,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 84 (OID 13865480)
-- Name: attributes; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE attributes FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 85 (OID 13865492)
-- Name: sessions; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE sessions (
    id character(32) NOT NULL,
    a_session bytea,
    lastupdated timestamp without time zone DEFAULT ('now'::text)::timestamp(6) with time zone NOT NULL
);


--
-- TOC entry 86 (OID 13865492)
-- Name: sessions; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE sessions FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 87 (OID 13911590)
-- Name: fm_classes; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_classes (
    id serial NOT NULL,
    name character varying(255) DEFAULT ''::character varying NOT NULL,
    description character varying(255) DEFAULT ''::character varying NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    disabled smallint DEFAULT 0::smallint NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 88 (OID 13911590)
-- Name: fm_classes; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_classes FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 101 (OID 13911590)
-- Name: fm_classes_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_classes_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 89 (OID 13911603)
-- Name: fm_classcustomfields; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_classcustomfields (
    id serial NOT NULL,
    "class" integer NOT NULL,
    customfield integer NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    sortorder smallint DEFAULT 0::smallint NOT NULL,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 90 (OID 13911603)
-- Name: fm_classcustomfields; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_classcustomfields FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 102 (OID 13911603)
-- Name: fm_classcustomfields_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_classcustomfields_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 91 (OID 13911613)
-- Name: fm_customfields; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_customfields (
    id serial NOT NULL,
    name character varying(200) DEFAULT ''::character varying NOT NULL,
    "type" character varying(200) DEFAULT ''::character varying NOT NULL,
    description character varying(200) DEFAULT ''::character varying NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 92 (OID 13911613)
-- Name: fm_customfields; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_customfields FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 103 (OID 13911613)
-- Name: fm_customfields_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_customfields_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 93 (OID 13911626)
-- Name: fm_articles; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_articles (
    id serial NOT NULL,
    name character varying(255) DEFAULT ''::character varying NOT NULL,
    summary character varying(255) DEFAULT ''::character varying NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    "class" integer DEFAULT 0 NOT NULL,
    parent integer DEFAULT 0 NOT NULL,
    uri character varying(255),
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 94 (OID 13911626)
-- Name: fm_articles; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_articles FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 104 (OID 13911626)
-- Name: fm_articles_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_articles_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 95 (OID 13911643)
-- Name: fm_customfieldvalues; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_customfieldvalues (
    id serial NOT NULL,
    customfield integer NOT NULL,
    name character varying(255) DEFAULT ''::character varying NOT NULL,
    description character varying(255) DEFAULT ''::character varying NOT NULL,
    sortorder integer DEFAULT 0 NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 96 (OID 13911643)
-- Name: fm_customfieldvalues; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_customfieldvalues FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 105 (OID 13911643)
-- Name: fm_customfieldvalues_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_customfieldvalues_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 97 (OID 13911655)
-- Name: fm_articlecfvalues; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_articlecfvalues (
    id serial NOT NULL,
    article integer NOT NULL,
    customfield integer NOT NULL,
    content text,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone,
    lastupdatedby integer DEFAULT 0 NOT NULL,
    lastupdated timestamp without time zone
);


--
-- TOC entry 98 (OID 13911655)
-- Name: fm_articlecfvalues; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_articlecfvalues FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 106 (OID 13911655)
-- Name: fm_articlecfvalues_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_articlecfvalues_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 99 (OID 13911667)
-- Name: fm_transactions; Type: TABLE; Schema: public; Owner: rt
--

CREATE TABLE fm_transactions (
    id serial NOT NULL,
    article integer DEFAULT 0 NOT NULL,
    changelog text DEFAULT ''::text NOT NULL,
    "type" character varying(64) DEFAULT ''::character varying NOT NULL,
    field character varying(64) DEFAULT ''::character varying NOT NULL,
    oldcontent text DEFAULT ''::text NOT NULL,
    newcontent text DEFAULT ''::text NOT NULL,
    creator integer DEFAULT 0 NOT NULL,
    created timestamp without time zone
);


--
-- TOC entry 100 (OID 13911667)
-- Name: fm_transactions; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_transactions FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 107 (OID 13911667)
-- Name: fm_transactions_id_seq; Type: ACL; Schema: public; Owner: rt
--

REVOKE ALL ON TABLE fm_transactions_id_seq FROM PUBLIC;


SET SESSION AUTHORIZATION 'rt';

--
-- TOC entry 108 (OID 13865232)
-- Name: attachments1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX attachments1 ON attachments USING btree (parent);


--
-- TOC entry 109 (OID 13865233)
-- Name: attachments2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX attachments2 ON attachments USING btree (transactionid);


--
-- TOC entry 110 (OID 13865234)
-- Name: attachments3; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX attachments3 ON attachments USING btree (parent, transactionid);


--
-- TOC entry 112 (OID 13865251)
-- Name: queues1; Type: INDEX; Schema: public; Owner: rt
--

CREATE UNIQUE INDEX queues1 ON queues USING btree (name);


--
-- TOC entry 114 (OID 13865263)
-- Name: links1; Type: INDEX; Schema: public; Owner: rt
--

CREATE UNIQUE INDEX links1 ON links USING btree (base, target, "type");


--
-- TOC entry 115 (OID 13865264)
-- Name: links4; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX links4 ON links USING btree ("type", localbase);


--
-- TOC entry 117 (OID 13865273)
-- Name: principals2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX principals2 ON principals USING btree (objectid);


--
-- TOC entry 119 (OID 13865281)
-- Name: groups1; Type: INDEX; Schema: public; Owner: rt
--

CREATE UNIQUE INDEX groups1 ON groups USING btree ("domain", instance, "type", id, name);


--
-- TOC entry 120 (OID 13865282)
-- Name: groups2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX groups2 ON groups USING btree ("type", instance, "domain");


--
-- TOC entry 123 (OID 13865308)
-- Name: transactions1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX transactions1 ON transactions USING btree (objecttype, objectid);


--
-- TOC entry 126 (OID 13865335)
-- Name: acl1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX acl1 ON acl USING btree (rightname, objecttype, objectid, principaltype, principalid);


--
-- TOC entry 129 (OID 13865353)
-- Name: cachedgroupmembers2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX cachedgroupmembers2 ON cachedgroupmembers USING btree (memberid);


--
-- TOC entry 130 (OID 13865354)
-- Name: cachedgroupmembers3; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX cachedgroupmembers3 ON cachedgroupmembers USING btree (groupid);


--
-- TOC entry 132 (OID 13865355)
-- Name: disgroumem; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX disgroumem ON cachedgroupmembers USING btree (groupid, memberid, disabled);


--
-- TOC entry 133 (OID 13865368)
-- Name: users1; Type: INDEX; Schema: public; Owner: rt
--

CREATE UNIQUE INDEX users1 ON users USING btree (name);


--
-- TOC entry 134 (OID 13865369)
-- Name: users2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX users2 ON users USING btree (name);


--
-- TOC entry 135 (OID 13865370)
-- Name: users3; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX users3 ON users USING btree (id, emailaddress);


--
-- TOC entry 136 (OID 13865371)
-- Name: users4; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX users4 ON users USING btree (emailaddress);


--
-- TOC entry 138 (OID 13865394)
-- Name: tickets1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX tickets1 ON tickets USING btree (queue, status);


--
-- TOC entry 139 (OID 13865395)
-- Name: tickets2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX tickets2 ON tickets USING btree ("owner");


--
-- TOC entry 140 (OID 13865396)
-- Name: tickets3; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX tickets3 ON tickets USING btree (effectiveid);


--
-- TOC entry 141 (OID 13865397)
-- Name: tickets4; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX tickets4 ON tickets USING btree (id, status);


--
-- TOC entry 142 (OID 13865398)
-- Name: tickets5; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX tickets5 ON tickets USING btree (id, effectiveid);


--
-- TOC entry 146 (OID 13865439)
-- Name: objectcustomfieldvalues1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX objectcustomfieldvalues1 ON objectcustomfieldvalues USING btree (customfield, objecttype, objectid, content);


--
-- TOC entry 147 (OID 13865440)
-- Name: objectcustomfieldvalues2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX objectcustomfieldvalues2 ON objectcustomfieldvalues USING btree (customfield, objecttype, objectid);


--
-- TOC entry 151 (OID 13865477)
-- Name: customfieldvalues1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX customfieldvalues1 ON customfieldvalues USING btree (customfield);


--
-- TOC entry 153 (OID 13865490)
-- Name: attributes1; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX attributes1 ON attributes USING btree (name);


--
-- TOC entry 154 (OID 13865491)
-- Name: attributes2; Type: INDEX; Schema: public; Owner: rt
--

CREATE INDEX attributes2 ON attributes USING btree (objecttype, objectid);


--
-- TOC entry 111 (OID 13865230)
-- Name: attachments_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY attachments
    ADD CONSTRAINT attachments_pkey PRIMARY KEY (id);


--
-- TOC entry 113 (OID 13865249)
-- Name: queues_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY queues
    ADD CONSTRAINT queues_pkey PRIMARY KEY (id);


--
-- TOC entry 116 (OID 13865261)
-- Name: links_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY links
    ADD CONSTRAINT links_pkey PRIMARY KEY (id);


--
-- TOC entry 118 (OID 13865271)
-- Name: principals_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY principals
    ADD CONSTRAINT principals_pkey PRIMARY KEY (id);


--
-- TOC entry 121 (OID 13865279)
-- Name: groups_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY groups
    ADD CONSTRAINT groups_pkey PRIMARY KEY (id);


--
-- TOC entry 122 (OID 13865293)
-- Name: scripconditions_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY scripconditions
    ADD CONSTRAINT scripconditions_pkey PRIMARY KEY (id);


--
-- TOC entry 124 (OID 13865306)
-- Name: transactions_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY transactions
    ADD CONSTRAINT transactions_pkey PRIMARY KEY (id);


--
-- TOC entry 125 (OID 13865323)
-- Name: scrips_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY scrips
    ADD CONSTRAINT scrips_pkey PRIMARY KEY (id);


--
-- TOC entry 127 (OID 13865333)
-- Name: acl_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY acl
    ADD CONSTRAINT acl_pkey PRIMARY KEY (id);


--
-- TOC entry 128 (OID 13865343)
-- Name: groupmembers_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY groupmembers
    ADD CONSTRAINT groupmembers_pkey PRIMARY KEY (id);


--
-- TOC entry 131 (OID 13865351)
-- Name: cachedgroupmembers_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY cachedgroupmembers
    ADD CONSTRAINT cachedgroupmembers_pkey PRIMARY KEY (id);


--
-- TOC entry 137 (OID 13865366)
-- Name: users_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY users
    ADD CONSTRAINT users_pkey PRIMARY KEY (id);


--
-- TOC entry 143 (OID 13865392)
-- Name: tickets_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY tickets
    ADD CONSTRAINT tickets_pkey PRIMARY KEY (id);


--
-- TOC entry 144 (OID 13865409)
-- Name: scripactions_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY scripactions
    ADD CONSTRAINT scripactions_pkey PRIMARY KEY (id);


--
-- TOC entry 145 (OID 13865423)
-- Name: templates_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY templates
    ADD CONSTRAINT templates_pkey PRIMARY KEY (id);


--
-- TOC entry 148 (OID 13865437)
-- Name: objectcustomfieldvalues_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY objectcustomfieldvalues
    ADD CONSTRAINT objectcustomfieldvalues_pkey PRIMARY KEY (id);


--
-- TOC entry 149 (OID 13865455)
-- Name: customfields_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY customfields
    ADD CONSTRAINT customfields_pkey PRIMARY KEY (id);


--
-- TOC entry 150 (OID 13865465)
-- Name: objectcustomfields_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY objectcustomfields
    ADD CONSTRAINT objectcustomfields_pkey PRIMARY KEY (id);


--
-- TOC entry 152 (OID 13865475)
-- Name: customfieldvalues_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY customfieldvalues
    ADD CONSTRAINT customfieldvalues_pkey PRIMARY KEY (id);


--
-- TOC entry 155 (OID 13865488)
-- Name: attributes_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY attributes
    ADD CONSTRAINT attributes_pkey PRIMARY KEY (id);


--
-- TOC entry 156 (OID 13865498)
-- Name: sessions_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY sessions
    ADD CONSTRAINT sessions_pkey PRIMARY KEY (id);


--
-- TOC entry 157 (OID 13911599)
-- Name: fm_classes_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_classes
    ADD CONSTRAINT fm_classes_pkey PRIMARY KEY (id);


--
-- TOC entry 158 (OID 13911609)
-- Name: fm_classcustomfields_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_classcustomfields
    ADD CONSTRAINT fm_classcustomfields_pkey PRIMARY KEY (id);


--
-- TOC entry 159 (OID 13911622)
-- Name: fm_customfields_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_customfields
    ADD CONSTRAINT fm_customfields_pkey PRIMARY KEY (id);


--
-- TOC entry 160 (OID 13911639)
-- Name: fm_articles_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_articles
    ADD CONSTRAINT fm_articles_pkey PRIMARY KEY (id);


--
-- TOC entry 161 (OID 13911651)
-- Name: fm_customfieldvalues_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_customfieldvalues
    ADD CONSTRAINT fm_customfieldvalues_pkey PRIMARY KEY (id);


--
-- TOC entry 162 (OID 13911663)
-- Name: fm_articlecfvalues_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_articlecfvalues
    ADD CONSTRAINT fm_articlecfvalues_pkey PRIMARY KEY (id);


--
-- TOC entry 163 (OID 13911680)
-- Name: fm_transactions_pkey; Type: CONSTRAINT; Schema: public; Owner: rt
--

ALTER TABLE ONLY fm_transactions
    ADD CONSTRAINT fm_transactions_pkey PRIMARY KEY (id);


SET SESSION AUTHORIZATION 'postgres';

--
-- TOC entry 3 (OID 2200)
-- Name: SCHEMA public; Type: COMMENT; Schema: -; Owner: postgres
--

COMMENT ON SCHEMA public IS 'Standard public schema';



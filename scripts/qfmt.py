def C(en_q, en, pl_q, pl, de_q, de):
    assert len(en) == len(pl) == len(de) == 4
    return (en_q, en, pl_q, pl, de_q, de)


def esc(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')
